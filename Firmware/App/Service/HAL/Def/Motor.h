/*
 * MIT License
 * Copyright (c) 2024 - 2026 _VIFEXTech
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
#ifndef __HAL_MOTOR_DEF_H
#define __HAL_MOTOR_DEF_H

#include <stdint.h>

namespace HAL {

typedef struct
{
    int16_t value[2];
} Motor_Info_t;

/**
 * Which kind of meter the two outputs drive. Only the timers of the active
 * mode are running, switching releases the ones of the previous mode.
 */
typedef enum : uint8_t {
    MOTOR_MODE_PWM = 0, /* two PWM meters, one pin each */
    MOTOR_MODE_SYNCHRO = 1, /* two synchro heads, three phase pins each */
} Motor_Mode_t;

/* DEVICEO_OBJECT_IOCMD_DEF(dir, size, type, number) */

#define MOTOR_IOCMD_SET_MODE DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, sizeof(HAL::Motor_Mode_t), 0, 0)

} // namespace HAL

#endif // __HAL_MOTOR_DEF_H
