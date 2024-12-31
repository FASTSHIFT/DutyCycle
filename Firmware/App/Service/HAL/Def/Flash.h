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
#ifndef __HAL_TEMPLATE_DEF_H
#define __HAL_TEMPLATE_DEF_H

#include <stdint.h>
#include <stddef.h>

namespace HAL {

/* clang-format off */

/* DEVICEO_OBJECT_IOCMD_DEF(dir, size, type, number) */

typedef struct
{
    uint8_t* addr;
    size_t len;
    size_t blk_size;
} Flash_Info_t;

#define FLASH_IOCMD_LOCK        DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 0, 0)
#define FLASH_IOCMD_UNLOCK      DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 1, 0)
#define FLASH_IOCMD_ERASE       DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, sizeof(size_t), 2, 0)
#define FLASH_IOCMD_GET_INFO    DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, sizeof(HAL::Flash_Info_t), 3, 0)
#define FLASH_IOCMD_SET_OFFSET  DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, sizeof(long), 4, 0)
#define FLASH_IOCMD_SAVE        DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 5, 0)

/* clang-format on */

} // namespace HAL

#endif // __HAL_TEMPLATE_DEF_H
