# DutyCycle 命令使用说明

## 概述
DutyCycle提供了一个命令行界面，用于与系统中的数据节点进行交互。以下是所有可用命令的详细使用说明。
核心代码位于：`Firmware/App/Service/DataProc/DP_Shell.cpp`

## 命令列表

1. **help**
2. **loglevel**
3. **ps**
4. **publish**
5. **clock**
6. **power**
7. **ctrl**
8. **alarm**
9. **kvdb**

---

### 1. help
**概述**：显示所有可用命令及其简要说明。

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

```

---

### 2. loglevel
**概述**：设置系统的日志级别。

**命令格式**：
```shell
loglevel <level>
```

- `<level>`：日志级别，取值范围为0~4。
    - 0: None
    - 1: Error
    - 2: Warning
    - 3: Info
    - 4: Debug

**示例**：
```shell
loglevel 3
```

**输出**：
```

```

---

### 3. ps
**概述**：显示系统栈最深使用量。

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

```

---

### 4. publish
**概述**：向指定主题发布数据。

**命令格式**：
```shell
publish <topic> [data]
```

- `<topic>`：要发布数据的主题名称。
- `[data]`：要发布的数据（可选）。

**示例**：
```shell
publish MyTopic "Hello, World!"
```

**输出**：
```

```

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

**示例**：
```shell
clock -c SET -y 2023 -m 10 -d 15 -H 14 -M 30 -S 45
```

**输出**：
```
Current clock: 2023-10-15 MON 14:30:45.0
New clock: 2023-10-15 14:30:45.0
```

---

### 6. power
**概述**：发送电源控制命令。

**命令格式**：
```shell
power -c <cmd>
```

- `-c <cmd>`：电源命令，可选值包括：
    - `SHUTDOWN`：关闭系统。
    - `REBOOT`：重启系统。

**示例**：
```shell
power -c SHUTDOWN
```

**输出**：
```

```

---

### 7. ctrl
**概述**：发送控制命令。

**命令格式**：
```shell
ctrl -c <cmd> [-H <hour>] [-M <motor>] [--mode <mode>]
```

- `-c <cmd>`：控制命令，可选值包括：
    - `SWEEP_TEST`：执行扫频测试。
    - `ENABLE_CLOCK_MAP`：启用时钟映射。
    - `SET_MOTOR_VALUE`：设置电机值。
    - `SET_CLOCK_MAP`：设置时钟映射。
    - `SET_MODE`：设置显示模式。
    - `SHOW_BATTERY_USAGE`：显示电池使用情况。
- `-H <hour>`：要设置的小时数。
- `-M <motor>`：要设置的电机值。
- `--mode <mode>`：显示模式，可选值包括：
    - `0`：功率因数表模式（cos-phi）。
    - `1`：线性电压、电流表模式（linear）。

**示例**：
```shell
ctrl -c SET_MOTOR_VALUE -M 100 --mode 1
```

**输出**：
```

```

---

### 8. alarm
**概述**：发送报警控制命令。

**命令格式**：
```shell
alarm -c <cmd> [-i <ID>] [-H <hour>] [-M <minute>] [-m <music>] [-f <filter>]
```

- `-c <cmd>`：报警命令，可选值包括：
    - `SET`：设置报警时间。
    - `LIST`：列出所有报警。
    - `SET_FILTER`：设置小时过滤器。
    - `PLAY_ALARM_MUSIC`：播放报警音乐。
- `-i <ID>`：报警ID。
- `-H <hour>`：小时。
- `-M <minute>`：分钟。
- `-m <music>`：音乐ID。
- `-f <filter>`：小时过滤器，格式为 `1,2,3,4`。

**示例**：
```shell
alarm -c SET -i 1 -H 7 -M 30 -m 5 -f 1,7,9
```

**输出**：
```

```

---

### 9. kvdb
**概述**：发送键值数据库控制命令。

**命令格式**：
```shell
kvdb -c <cmd> -k <key> [value]
```

- `-c <cmd>`：KVDB命令，可选值包括：
    - `DEL`：删除指定的键。
    - `LIST`：列出所有键。
    - `SAVE`：保存键值对。
- `-k <key>`：键名。
- `[value]`：要保存的值（仅在 `SAVE` 命令中使用）。

**示例**：
```shell
kvdb -c SAVE -k myKey myValue
```

**输出**：
```

```

---

## 注意事项
- 所有命令在解析错误或指定参数无效时，将返回错误信息。
- `SAVE` 命令需要同时指定键名和值。
- `DEL` 命令仅需要指定键名。
- `LIST` 命令不需要指定任何参数。
- 时钟命令的过滤器格式为逗号分隔的小时列表，例如 `1,2,3,4`。
