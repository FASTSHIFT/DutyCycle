# DutyCycle
Work-Life Duty Cycle Clock

## 下载
```bash
git clone https://github.com/FASTSHIFT/DutyCycle.git --recursive
```

## 编译
进入目录：`Firmware/Vendor/Artery/Platform/AT32F421/MDK-ARM`

打开`proj.uvprojx`文件，使用Keil v5.25以上版本进行编译。

## 使用

[操作命令](./Document/Commands.md)

- **时间映射关系**

  | 数值      | 时间  |
  | --------- | ----- |
  | 0.5（上） | 05:00 |
  | 0.7（上） | 07:00 |
  | 0.9（上） | 09:00 |
  | 1.0       | 12:00 |
  | 0.9（下） | 21:00 |
  | 0.7（下） | 01:00 |
  | 0.5（下） | 04:59 |

## 常见问题
