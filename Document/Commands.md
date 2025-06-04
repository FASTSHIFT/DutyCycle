# DutyCycle 命令使用说明

## 概述
DutyCycle提供了一个命令行界面，用于与系统中的数据节点进行交互。以下是所有可用命令的详细使用说明。
核心代码位于：`Firmware/App/Service/DataProc/DP_Shell.cpp`

## 命令列表

| 命令     | 注释                 |
|----------|----------------------|
| **help** | 显示帮助信息         |
| **loglevel** | 设置日志级别       |
| **ps** | 显示栈使用最大深度         |
| **publish** | 发布消息（仅供调试使用） |
| **clock** | 显示或设置系统时钟   |
| **power** | 控制电源状态   |
| **ctrl** | 控制设备功能         |
| **alarm** | 闹钟设置       |
| **kvdb** | 数据库操作（仅供调试使用）    |

---

### 1. help
**概述**：显示所有可用命令。

**命令格式**：
```shell
help
```

**示例**：
```shell
help
```

**输出**：
```
Available Commands:
help
loglevel
ps
publish
clock
power
ctrl
alarm
kvdb
```

---

### 2. loglevel
**概述**：设置系统的日志级别。

**命令格式**：
```shell
loglevel <level>
```

- `<level>`：日志级别，取值范围为0~4。
    - 0: Trace
    - 1: Info （默认值）
    - 2: Warning
    - 3: Error
    - 4: Off

**示例**：

设置日志级别为3（仅打印错误等级日志）。
```shell
loglevel 3
```

**输出**：
无，后面将仅打印错误等级日志。

---

### 3. ps
**概述**：显示系统栈最深使用量，需要确保日志等级为1(Info)及以下。

**命令格式**：
```shell
ps
```

**示例**：
```shell
ps
```

**输出**：
```
[INFO] Memory_DumpStackInfo: Stack: 99% used (total: 1536, free: 4)
```
栈使用量为99%，总大小为1536字节，剩余大小为4字节。

---

### 4. publish
**概述**：向订阅`DP_Shell`的节点发布消息，仅供内部调试使用。

**命令格式**：
```shell
publish <msg1> <msg2> ...
```

- `<msg1>`：消息内容1。
- `<msg2>`：消息内容2。
- 等等。

**示例**：
```shell
publish aaa bbb
```

**输出**：
```shell
publish finished: -3
```
消息发布结束，返回值为`-3`。

---

### 5. clock
**概述**：显示当前时钟，并可以设置新的时钟时间。

**命令格式**：
```shell
clock [-c <cmd>] [-y <year>] [-m <month>] [-d <day>] [-H <hour>] [-M <minute>] [-S <second>]
```

- `-c <cmd>`：时钟命令，可选值包括：
    - `SET`：设置新的时钟时间。
- `-y <year>`：年份。
- `-m <month>`：月份。
- `-d <day>`：日期。
- `-H <hour>`：小时。
- `-M <minute>`：分钟。
- `-S <second>`：秒。

**示例1**：

显示当前时间，不设置新的时间。
```shell
clock
```

**输出**：
```shell
Current clock: 2025-04-25 FRI 14:17:32.18
#ERROR-PARAM:command is null
#ERROR-TYPE:PARSING
```
当前时间为2025年4月25日星期五14时17分32秒18毫秒。

**示例2**：

设置新的时钟时间为2025年4月25日14时15分10秒。
```shell
clock -c SET -y 2025 -m 4 -d 25 -H 14 -M 15 -S 10
```

**输出**：
```
Current clock: 2025-04-25 FRI 14:01:07.260
New clock: 2025-04-25 14:15:10.0
```
当前时间为2025年4月25日14时01分07秒260毫秒，设置后的时间为2025年4月25日14时15分10秒0毫秒。

**示例3**：

使用`config_clock.py`脚本自动配置时间，打开PC终端（非串口终端），使用脚本请确保安装`python3.x`及`pyserial`模块:
````shell
pip install -r Tools/requirements.txt
````

运行以下命令自动设置时间：
```shell
python Tools/config_clock.py
```

或者添加`-p`参数手动指定串口号：
```shell
python Tools/config_clock.py -p COM3
```

**输出**：
```
No specific port was provided. Using the first available port: COM3
Serial port COM3 opened with baud rate 115200 and timeout 1 seconds
Sending command: clock -c SET -y 2025 -m 4 -d 26 -H 0 -M 3 -S 14
Received data:
clock -c SET -y 2025 -m 4 -d 26 -H 0 -M 3 -S 14
Serial port closed
```
脚本使用自动扫描串口模式，显示了回显信息，表示自动配置时间为2025年4月26日0点3分14秒。

---

### 6. power
**概述**：电源控制。注意：关闭或重启系统后，RTC（实时时钟）也会关闭，再次启动后需要重新设置时间，系统启动的默认时间为固件编译时间。

**命令格式**：
```shell
power -c <cmd>
```

- `-c <cmd>`：电源控制命令，可选值包括：
    - `SHUTDOWN`：关闭系统。
    - `REBOOT`：重启系统。

**示例**：
```shell
power -c SHUTDOWN
```

**输出**：
```
[WARN] HAL::Power::onIoctl: Power off!
```
系统下电。

---

### 7. ctrl
**概述**：发送控制命令。

**命令格式**：
```shell
ctrl -c <cmd> [-H <hour>] [-M <motor>] [--mode <mode>]
```

- `-c <cmd>`：控制命令，可选值包括：
    - `SWEEP_TEST`：执行指针扫动测试。
    - `ENABLE_CLOCK_MAP`：启用时钟映射。
    - `LIST_CLOCK_MAP`：列出时钟映射。
    - `SET_MOTOR_VALUE`：设置电机值。
    - `SET_CLOCK_MAP`：设置时钟映射。
    - `SET_MODE`：设置显示模式。
    - `SHOW_BATTERY_USAGE`：显示电池使用情况。
- `-H <hour>`：要设置的小时数。
- `-M <motor>`：要设置的电机值。
- `--mode <mode>`：显示模式，可选值包括：
    - `0`：功率因数表模式（cos-phi）。
    - `1`：电压、电流（线性）表模式（linear）。

**示例**：
设置电机值为100。
```shell
ctrl -c SET_MOTOR_VALUE -M 100
```

**输出**：
```
[INFO] DP_Ctrl::onTimer: Motor value reached: 100
```
电机已设置值为100。

---

### 8. alarm
**概述**：设置闹钟和正点报时功能。

**命令格式**：
```shell
alarm -c <cmd> [-i <ID>] [-H <hour>] [-M <minute>] [-m <music>] [-f <filter>]
```

- `-c <cmd>`：闹钟命令，可选值包括：
    - `SET`：设置闹钟时间。
    - `LIST`：列出所有闹钟。
    - `SET_FILTER`：设置小时过滤器。
    - `PLAY_ALARM_MUSIC`：播放闹钟音乐。
    - `PLAY_ALARM_HOURLY`：播放正点报时音乐。
    - `PLAY_TONE`：播放指定频率的音效。
- `-i <ID>`：闹钟ID。范围为0~2。
- `-H <hour>`：小时。
- `-M <minute>`：分钟。
- `-m <music>`：音乐ID。范围为0~2。
- `-f <filter>`：正点报时小时过滤器，格式为 `1,2,3,4`。
- `--freq <freq>`：音效频率（单位：赫兹），默认值为1000赫兹。
- `--duration <duration>`：音效持续时间（单位：毫秒），默认值为1000毫秒。

**示例1**：
设置闹钟0的时间为7:30，音乐ID为1。
```shell
alarm -c SET -i 0 -H 7 -M 30 -m 1
```

**输出**：
无，使用`LIST`指令查看是否生效。

**示例2**：
列出所有闹钟信息。
```shell
alarm -c LIST
```

**输出**：
```
[INFO] DP_Alarm::listAlarms: Hourly alarm filter: 0x00FFDC00
[INFO] DP_Alarm::listAlarms: Alarm 0: 07:30, Music ID: 1
[INFO] DP_Alarm::listAlarms: Alarm 1: 18:20, Music ID: 1
[INFO] DP_Alarm::listAlarms: Alarm 2: 21:00, Music ID: 2
```
报时过滤器为`0x00FFDC00`；闹钟0为7:30，音乐ID为1；闹钟1为18:20，音乐ID为1；闹钟2为21:00，音乐ID为2。

---

### 9. kvdb
**概述**：数据库操作（仅供调试使用）。

**命令格式**：
```shell
kvdb -c <cmd> -k <key>
```

- `-c <cmd>`：KVDB命令，可选值包括：
    - `DEL`：删除指定的键。
    - `LIST`：列出所有键。
    - `SAVE`：保存（此）。
- `-k <key>`：键名。

**示例**：
列出所有键。
```shell
kvdb -c LIST
```

**输出**：
```
[INFO] DP_KVDB::onSet: Key list:
[INFO] DP_KVDB::onSet: [0]:
[INFO] DP_KVDB::onSet: 	name = '_hourMotorMap'
[INFO] DP_KVDB::onSet: 	status = 2
[INFO] DP_KVDB::onSet: 	crc_is_ok = 1
[INFO] DP_KVDB::onSet: 	name_len = 13
[INFO] DP_KVDB::onSet: 	magic = 0x1
[INFO] DP_KVDB::onSet: 	len = 108
[INFO] DP_KVDB::onSet: 	value_len = 50
[INFO] DP_KVDB::onSet: 	addr.start = 0x1452
[INFO] DP_KVDB::onSet: 	addr.value = 0x1508
[INFO] DP_KVDB::onSet: [1]:
[INFO] DP_KVDB::onSet: 	name = '_alarmParam'
[INFO] DP_KVDB::onSet: 	status = 2
[INFO] DP_KVDB::onSet: 	crc_is_ok = 1
[INFO] DP_KVDB::onSet: 	name_len = 11
[INFO] DP_KVDB::onSet: 	magic = 0x1
[INFO] DP_KVDB::onSet: 	len = 68
[INFO] DP_KVDB::onSet: 	value_len = 16
[INFO] DP_KVDB::onSet: 	addr.start = 0x1696
[INFO] DP_KVDB::onSet: 	addr.value = 0x1748
```
