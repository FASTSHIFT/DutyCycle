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
#define CONFIG_LOG_SERIAL          Serial

/* Motor */
#define CONFIG_MOTOR_OUT1_PIN      PA2
#define CONFIG_MOTOR_OUT2_PIN      PA3

/**
 * Synchro, 3 phase PWM per head, used when the "Motor" device is switched to
 * MOTOR_MODE_SYNCHRO (ctrl -c SET_MODE --mode 2). Head A is value[0] and head B
 * is value[1].
 * The three phases of one head sit on different timers, which is fine as long
 * as all of them share the same carrier. Keep clear of PA6/PA7/PB0/PB1/PB2
 * (HAL_PinRemap), PA9/PA10 (debug serial) and PA13/PA14 (SWD).
 * Tick runs on TIM6 and Tone on TIM16, which leaves TIM1/TIM3/TIM14/TIM15 to PWM.
 */
#define CONFIG_SYNCHRO_A_U_PIN     PA2  /* TIM15 CH1 */
#define CONFIG_SYNCHRO_A_V_PIN     PA3  /* TIM15 CH2 */
#define CONFIG_SYNCHRO_A_W_PIN     PA4  /* TIM14 CH1 */
#define CONFIG_SYNCHRO_B_U_PIN     PB4  /* TIM3  CH1 */
#define CONFIG_SYNCHRO_B_V_PIN     PB5  /* TIM3  CH2 */
#define CONFIG_SYNCHRO_B_W_PIN     PA8  /* TIM1  CH1 */

#define CONFIG_SYNCHRO_CARRIER_HZ  20000 /* SYNCHRO_RES * carrier must not exceed the timer clock */
#define CONFIG_SYNCHRO_AMPLITUDE   100   /* percent */

/* Buzzer */
#define CONFIG_BUZZ_V1_2_PIN       PB1
#define CONFIG_BUZZ_V1_1_PIN       PB2
#define CONFIG_BUZZ_PIN            HAL_PinRemap(CONFIG_BUZZ_V1_2_PIN)

/* Battery */
#define CONFIG_BATT_DET_V1_2_PIN   PB0
#define CONFIG_BATT_DET_V1_1_PIN   PB1
#define CONFIG_BATT_DET_PIN        HAL_PinRemap(CONFIG_BATT_DET_V1_2_PIN)
#define CONFIG_PWR_EN_V1_2_PIN     PA6
#define CONFIG_PWR_EN_V1_1_PIN     PA7
#define CONFIG_PWR_EN_PIN          HAL_PinRemap(CONFIG_PWR_EN_V1_2_PIN)

/* Button */
#define CONFIG_BUTTON_SEL_V1_2_PIN PA7
#define CONFIG_BUTTON_SEL_V1_1_PIN PB0
#define CONFIG_BUTTON_SEL_PIN      HAL_PinRemap(CONFIG_BUTTON_SEL_V1_2_PIN)

/* Memory */
#define CONFIG_MEMORY_STACK_INFO  1 /* Reuqire MPU Disabled */
#define CONFIG_MEMORY_HEAP_INFO   1

/* WatchDog */
#define CONFIG_WATCHDOG_TIMEOUT   10 /* seconds */

/* clang-format on */

#endif /* __HAL_CONFIG_H */
