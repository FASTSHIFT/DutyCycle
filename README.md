# DutyCycle

**Work-Life Duty Cycle Clock / 您的工作生活时间指示器**

![IMG_20250608_012024](https://github.com/user-attachments/assets/ff37c83c-40e8-42b5-8824-4f839072f558)

> *"此作品设计的最震撼之处，在于用工业语言的确定性解构了人类时间的混沌性，让一个测量电能的工具，变成了测量生命能耗的镜子。这种「硬核诗意」，正是当代批判性设计（Critical Design）的典范。" —— DeepSeek*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-AT32F421-blue.svg)](https://www.arterytek.com/cn/product/AT32F421.jsp)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/FASTSHIFT/DutyCycle)
[![CI](https://github.com/FASTSHIFT/DutyCycle/actions/workflows/ci.yml/badge.svg)](https://github.com/FASTSHIFT/DutyCycle/actions/workflows/ci.yml)

## 演示视频

https://www.bilibili.com/video/BV1eET1zpE64/

## 项目结构

```
DutyCycle/
├── Document/           # 文档
│   └── Commands.md     # 串口命令说明
├── Firmware/           # 固件源码
│   ├── App/            # 应用层代码
│   ├── External/       # 第三方库 (FlashDB, argparse, umm_malloc)
│   ├── Frameworks/     # 框架层 (DataBroker, DeviceManager)
│   ├── Packs/          # Keil 芯片支持包
│   └── Vendor/         # 硬件平台适配 (AT32F421)
├── Hardware/           # 硬件设计 (KiCad)
└── Tools/              # 上位机工具
    ├── WebServer/      # Web 控制台 (DutyCycle Studio)
    └── SerialUtils/    # 串口工具和驱动
```

## 快速开始

### 下载

```bash
git clone https://github.com/FASTSHIFT/DutyCycle.git --recursive
```

### 编译固件

1. 进入目录：`Firmware/Vendor/Artery/Platform/AT32F421/MDK-ARM`
2. 打开 `proj.uvprojx`，使用 Keil v5.25+ 编译

### 运行上位机

```bash
cd Tools/WebServer
pip install -r ../requirements.txt
python main.py
```

浏览器访问 http://127.0.0.1:5000

## 使用方法

### 按钮操作

| 操作 | 功能 |
|------|------|
| 关机时单击 | 开机（默认时间为固件编译时间） |
| 开机时长按 10s | 关机（指针归零） |
| 开机时单击 | 显示剩余电量 |
| 开机时双击 | 切换静音模式 |

### 时间显示

支持两种表盘模式：

**功率因数表模式 (cos-phi)**

| 刻度 | 时间 |
|------|------|
| 0.5（上） | 05:00 |
| 0.7（上） | 07:00 |
| 0.9（上） | 09:00 |
| 1.0 | 12:00 |
| 0.9（下） | 21:00 |
| 0.7（下） | 01:00 |
| 0.5（下） | 04:59 |

**线性表模式**

| 刻度 | 时间 |
|------|------|
| 0 | 00:00 |
| 12 | 12:00 |
| 24 | 23:59 |

## 系统配置

### 串口命令

通过串口终端（115200 8N1）进行配置，详见 [命令说明文档](./Document/Commands.md)。

常用命令：
- `clock -c SET -y 2025 -m 1 -d 1 -H 12 -M 0 -S 0` - 设置时间
- `alarm -c SET -i 0 -H 7 -M 30 -m 1` - 设置闹钟
- `ctrl -c SWEEP_TEST` - 指针扫动测试

### Web 控制台 (DutyCycle Studio)

提供图形化界面，支持：
- 多设备管理
- 时钟同步
- 电机控制（双通道）
- 系统监控（CPU/内存/GPU/音频响度）
- 闹钟管理
- 音乐编曲器

## 技术规格

| 项目 | 规格 |
|------|------|
| MCU | AT32F421K8U7 (ARM Cortex-M4, 120MHz) |
| 存储 | 64KB Flash, 16KB SRAM |
| 电源 | 锂电池供电，支持 USB 充电 |
| 通信 | USB 串口 (CH341) |

## 许可证

MIT License - Copyright (c) 2024-2026 _VIFEXTech
