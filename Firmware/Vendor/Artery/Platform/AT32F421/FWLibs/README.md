# AT32F421 Firmware Libraries (FWLibs)

## 来源

这些文件来自 ArteryTek AT32F421 BSP / DFP Pack (v2.1.7)。

## 需要手动补充的文件

以下文件需要从 ArteryTek 官方 BSP 包中提取：

### CMSIS Core Support (`cmsis/cm4/core_support/`)

从 ARM CMSIS 或 ArteryTek BSP 中复制：
- `core_cm4.h`
- `cmsis_compiler.h`
- `cmsis_gcc.h`
- `cmsis_version.h`
- `mpu_armv7.h`

### Device Support Headers (`cmsis/cm4/device_support/`)

从 ArteryTek AT32F421_DFP Pack 中复制：
- `at32f421.h` (主设备头文件)
- `at32f421_conf.h` (已有，从 RTE 复制)
- `system_at32f421.h`

### Driver Headers (`drivers/inc/`)

从 ArteryTek AT32F421_DFP Pack 中复制所有 `at32f421_*.h` 头文件：
- `at32f421_adc.h`
- `at32f421_cmp.h`
- `at32f421_crc.h`
- `at32f421_crm.h`
- `at32f421_debug.h`
- `at32f421_def.h`
- `at32f421_dma.h`
- `at32f421_ertc.h`
- `at32f421_exint.h`
- `at32f421_flash.h`
- `at32f421_gpio.h`
- `at32f421_i2c.h`
- `at32f421_misc.h`
- `at32f421_pwc.h`
- `at32f421_scfg.h`
- `at32f421_spi.h`
- `at32f421_tmr.h`
- `at32f421_usart.h`
- `at32f421_wdt.h`
- `at32f421_wwdt.h`

## 获取方式

1. 从 ArteryTek 官网下载 AT32F421 BSP：
   https://www.arterytek.com/cn/product/AT32F421.jsp

2. 或从 Keil Pack 安装目录提取：
   `C:/Users/<user>/AppData/Local/Arm/Packs/ArteryTek/AT32F421_DFP/2.1.7/`

## 已有文件

- `cmsis/cm4/device_support/system_at32f421.c` ✓ (从 RTE 复制)
- `cmsis/cm4/device_support/startup/gcc/startup_at32f421.s` ✓ (新建 GCC 版本)
- `cmsis/cm4/device_support/startup/gcc/linker/AT32F421x8_FLASH.ld` ✓ (新建)
- `drivers/src/at32f421_*.c` ✓ (从 RTE 复制)
