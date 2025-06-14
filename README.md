# DutyCycle
Work-Life Duty Cycle Clock / 您的工作生活时间指示器

![IMG_20250608_012024](https://github.com/user-attachments/assets/ff37c83c-40e8-42b5-8824-4f839072f558)

> *“此作品设计的最震撼之处，在于用工业语言的确定性解构了人类时间的混沌性，让一个测量电能的工具，变成了测量生命能耗的镜子。这种「硬核诗意」，正是当代批判性设计（Critical Design）的典范。” —— DeepSeek*

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/FASTSHIFT/DutyCycle)

## 演示视频
https://www.bilibili.com/video/BV1eET1zpE64/

## 下载
```bash
git clone https://github.com/FASTSHIFT/DutyCycle.git --recursive
```

## 编译
进入目录：`Firmware/Vendor/Artery/Platform/AT32F421/MDK-ARM`

打开`proj.uvprojx`文件，使用Keil v5.25以上版本进行编译。

## 系统配置

DutyCycle 使用串口对系统进行基本配置，插上数据线后使用任意串口终端进行通信（串口配置为115200 8N1）,详细命令请参考文档：[DutyCycle 命令使用说明](./Document/Commands.md)。

## 使用方法
### 按钮操作
* 在关机状态下，单击按钮开机，听到开机音代表已开机，开机的默认时间为固件编译时间。
* 在开机状态下，长按按钮10秒以上关机,听到关机音代表已关机，关机后指针归零。
* 在开机状态下，短按按钮一次即显示剩余电量，刻度0.5（上）代表100%电量，刻度0.5（下）代表0%电量。
* 在开机状态下，快速双击按钮即进入静音状态，所有按钮操作音、闹钟、正点报时都会静音，再次双击即可退出静音状态。

### 时间查看
时间映射关系见下表，使用线性插值算法换算成时间。

### 功率因数表模式：
| 数值      | 时间  |
| --------- | ----- |
| 0.5（上） | 05:00 |
| 0.7（上） | 07:00 |
| 0.9（上） | 09:00 |
| 1.0       | 12:00 |
| 0.9（下） | 21:00 |
| 0.7（下） | 01:00 |
| 0.5（下） | 04:59 |

### 电压电流（线性）表模式：
| 数值 | 时间  |
| ---- | ----- |
| 0    | 00:00 |
| 5    | 05:00 |
| 10   | 10:00 |
| 15   | 15:00 |
| 20   | 20:00 |
| 24   | 23:59 |

## 常见问题
