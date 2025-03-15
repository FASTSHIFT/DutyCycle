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

/* Motor */
#define CONFIG_MOTOR_OUT1_PIN     PA2
#define CONFIG_MOTOR_OUT2_PIN     PA3

/* Buzzer */
#define CONFIG_BUZZ_PIN           PB1

/* Battery */
#define CONFIG_BATT_DET_PIN       PB0
#define CONFIG_PWR_EN_PIN         PA6

/* Button */
#define CONFIG_BUTTON_SEL_PIN     PA7

/* Memory */
#define CONFIG_MEMORY_STACK_INFO  1 /* Reuqire MPU Disabled */
#define CONFIG_MEMORY_HEAP_INFO   1

/* WatchDog */
#define CONFIG_WATCHDOG_TIMEOUT   10 /* seconds */

/* clang-format on */

#endif /* __HAL_CONFIG_H */
