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
#ifndef __HAL_POWER_DEF_H
#define __HAL_POWER_DEF_H

#include <stdint.h>

namespace HAL {

/* clang-format off */

/* DEVICEO_OBJECT_IOCMD_DEF(dir, size, type, number) */

#define POWER_IOCMD_WFI         DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 0, 0)
#define POWER_IOCMD_POWER_OFF   DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 1, 0)
#define POWER_IOCMD_REBOOT      DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 2, 0)
#define POWER_IOCMD_GOTO_ISP    DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 3, 0)

} // namespace HAL

#endif // __HAL_POWER_DEF_H
