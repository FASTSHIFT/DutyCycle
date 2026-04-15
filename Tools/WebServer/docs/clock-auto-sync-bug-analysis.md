# 时钟自动同步功能 Bug 分析与修复方案

## 问题现象

时钟自动同步功能无法工作，只有手动点击同步按钮才能成功同步时钟。

## 代码调用链分析

### 手动同步（正常工作）

```
前端 syncClock() → POST /api/clock → config_clock(device) → serial_write(device, command)
```

`serial_write` 将写命令入队到 worker 线程并等待完成，此时调用方是 Flask 请求线程，worker 线程空闲可以处理队列，流程正常。

### 自动同步（不工作）

```
DeviceWorker._worker_loop()
  → TimerManager.tick()
    → check_clock_sync() 回调
      → config_clock(device)
        → serial_write(device, command)
          → worker.enqueue_and_wait("write", command)  ← 死锁！
```

## 根因分析

### Bug 1：Worker 线程自死锁（致命）

`check_clock_sync()` 回调由 `TimerManager.tick()` 触发，而 `tick()` 运行在 `DeviceWorker._worker_loop()` 内部（即 worker 线程中）。

回调内部调用 `config_clock(device)` → `serial_write(device, command)` → `worker.enqueue_and_wait("write", command)`。

`enqueue_and_wait` 的实现是：将命令放入队列，然后 **阻塞等待** `done_event`。但 `done_event` 只有在 worker 线程处理完该队列项后才会被 set。而此时 worker 线程正被 `enqueue_and_wait` 阻塞，无法继续处理队列 —— **经典的自死锁**。

结果：`enqueue_and_wait` 等待 2 秒超时后返回 `False`，`serial_write` 返回 `"Command timeout"` 错误，同步失败。

相关代码位置：
- `routes.py:86` — `check_clock_sync()` 调用 `config_clock(device)`
- `device.py:233` — `config_clock` 调用 `serial_write(device, command)`
- `serial_utils.py:65` — `serial_write` 调用 `worker.enqueue_and_wait()`
- `device_worker.py:100` — `enqueue_and_wait` 阻塞等待 `done_event`

### Bug 2：`restore_state()` 未启动时钟同步定时器

`main.py` 的 `restore_state()` 函数在服务启动时自动恢复串口连接和监控，但**没有调用 `setup_clock_sync_timer(device)`**。

`setup_clock_sync_timer` 仅在 `routes.py` 的 `api_connect()` 路由中被调用（即用户通过 Web 界面手动连接时）。这意味着：

- 服务重启后自动重连的设备，即使 `auto_sync_clock=True`，也不会启动定时器
- 只有用户手动断开再重连，定时器才会被创建

相关代码位置：
- `main.py:108-155` — `restore_state()` 缺少 `setup_clock_sync_timer` 调用
- `routes.py:236` — 仅在 `api_connect` 中调用

## 修复方案

### 修复 Bug 1：在 worker 线程内使用直接写入代替队列写入

`check_clock_sync()` 回调已经运行在 worker 线程中，应该使用 `serial_write_direct` 直接写入串口，而不是通过 `serial_write` 入队再等待。

这与 `monitor.py` 中监控定时器回调的做法一致（监控回调使用 `serial_write_direct` 发送电机控制命令）。

```python
# routes.py - check_clock_sync() 修复
def check_clock_sync():
    if not device.auto_sync_clock:
        return
    if device.ser is None:
        return

    need_sync = True
    if device.last_sync_time:
        try:
            last_sync = datetime.fromisoformat(device.last_sync_time)
            hours_since = (datetime.now() - last_sync).total_seconds() / 3600
            need_sync = hours_since >= 24
        except Exception:
            pass

    if need_sync:
        logger.info(f"[{device.name}] Auto clock sync triggered")
        now = datetime.now()
        command = (
            f"clock -c SET -y {now.year} -m {now.month} -d {now.day}"
            f" -H {now.hour} -M {now.minute} -S {now.second}\r\n"
        )
        serial_write_direct(device, command)
        device.last_sync_time = now.isoformat()
        state.save_config()
        logger.info(f"[{device.name}] Clock synced at {device.last_sync_time}")
```

需要在 `routes.py` 顶部增加导入：

```python
from serial_utils import serial_write_direct  # 新增
```

### 修复 Bug 2：在 `restore_state()` 中启动时钟同步定时器

```python
# main.py - restore_state() 中增加时钟同步定时器启动
from routes import setup_clock_sync_timer  # 新增导入

def restore_state():
    for device_id, device in state.devices.items():
        if not device.auto_connect or not device.port:
            continue

        # ... 现有的自动连接逻辑 ...

        device.ser = ser
        logger.info(f"[{device.name}] Auto-connected to {device.port}")

        # 启动时钟同步定时器（新增）
        if device.auto_sync_clock:
            setup_clock_sync_timer(device)

        # ... 现有的自动监控逻辑 ...
```

## 影响范围

- `routes.py`：修改 `check_clock_sync` 回调实现，增加 `serial_write_direct` 导入
- `main.py`：在 `restore_state` 中增加 `setup_clock_sync_timer` 调用
- `static/js/app.js`：增加 60 秒轮询刷新"上次同步时间"显示

修改不影响手动同步功能和其他现有功能。

### Bug 3：前端不会自动刷新"上次同步时间"

自动同步在后端 worker 线程中完成，前端无感知。`updateLastSyncTime()` 仅在页面初始化 `refreshStatus()` 和手动同步 `syncClock()` 成功后被调用，没有轮询机制来获取后端自动同步后更新的时间。

### 修复 Bug 3：前端增加低频轮询刷新同步时间

在 `app.js` 中增加 60 秒间隔的轮询，从 `/api/status` 拉取 `last_sync_time` 并更新显示：

```javascript
let syncTimeInterval = null;

function startSyncTimePolling() {
  if (syncTimeInterval) clearInterval(syncTimeInterval);
  syncTimeInterval = setInterval(pollSyncTime, 60000); // 60秒轮询
}

async function pollSyncTime() {
  if (!isConnected) return;
  const result = await api('/status');
  if (result.success && result.last_sync_time) {
    updateLastSyncTime(result.last_sync_time);
  }
}
```

在 `DOMContentLoaded` 中调用 `startSyncTimePolling()` 启动轮询。
