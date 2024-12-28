/*
 * MIT License
 * Copyright (c) 2024 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __HAL_CONFIG_H
#define __HAL_CONFIG_H

/* clang-format off */

/* Logger Serial */
#define CONFIG_LOG_SERIAL         Serial

/* Radar */
#define CONFIG_RADAR_PWR_EN_PIN   PB8
#define CONFIG_RADAR_SERIAL       Serial2
#define CONFIG_RADAR_OUT_PIN      PA1

/* Battery */
#define CONFIG_BATT_DET_PIN       PA0
#define CONFIG_PWR_EN_PIN         PA15

/* Button */
#define CONFIG_BUTTON_SEL_PIN     PB3

/* IMU */
#define CONFIG_IMU_INT1_PIN       PA4
#define CONFIG_IMU_INT2_PIN       PB1
#define CONFIG_IMU_CS_PIN         PB0
#define CONFIG_IMU_SCK_PIN        PA5
#define CONFIG_IMU_MISO_PIN       PA6
#define CONFIG_IMU_MOSI_PIN       PA7
#define CONFIG_IMU_SPI            0

/* LED */
#define CONFIG_LED_CTRL_PIN       PA8

/* Memory */
#define CONFIG_MEMORY_STACK_INFO  1 /* Reuqire MPU Disabled */
#define CONFIG_MEMORY_HEAP_INFO   1

/* WatchDog */
#define CONFIG_WATCHDOG_TIMEOUT   0 /* seconds */

/* clang-format on */

#endif /* __HAL_CONFIG_H */
