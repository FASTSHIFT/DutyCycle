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
#ifndef __HAL_DEF_H
#define __HAL_DEF_H

#include <stddef.h>
#include <stdint.h>

namespace HAL {

/* clang-format off */

/* DEVICEO_OBJECT_IOCMD_DEF(dir, size, type, number) */

typedef struct {
    void (*callback)(void*);
    void* arg;
} HAL_SimpleCallback_t;

/*********************
 *      Flash
 *********************/

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

/*********************
 *      Battery
 *********************/

typedef struct
{
    uint16_t voltage;
    uint8_t level;
    bool isReady;
    bool isCharging;
} Battery_Info_t;

#define BATTERY_IOCMD_SLEEP     DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 0, 0)

/*********************
 *      Power
 *********************/

#define POWER_IOCMD_WFI         DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 0, 0)
#define POWER_IOCMD_POWER_OFF   DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 1, 0)
#define POWER_IOCMD_REBOOT      DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 2, 0)

/*********************
 *      WatchDog
 *********************/

typedef HAL_SimpleCallback_t WatchDog_Callback_t;

#define WATCHDOG_IOCMD_SET_TIMEOUT  DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, sizeof(int), 0, 0)
#define WATCHDOG_IOCMD_ENABLE       DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 1, 0)
#define WATCHDOG_IOCMD_KEEP_ALIVE   DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 2, 0)
#define WATCHDOG_IOCMD_SET_CALLBACK DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, sizeof(HAL::WatchDog_Callback_t), 3, 0)

/*********************
 *      Button
 *********************/

typedef struct
{
    union
    {
        uint32_t value;
        struct
        {
            uint32_t ok : 1;
            uint32_t up : 1;
            uint32_t down : 1;
        } key;
    };
} Button_Info_t;

/*********************
 *      Clock
 *********************/

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
} Clock_Info_t;

#define CLOCK_IOCMD_CALIBRATE   DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, sizeof(HAL::Clock_Info_t), 0, 0)

/*********************
 *      Tick
 *********************/

#define TICK_IOCMD_START    DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, sizeof(uint32_t), 0, 0)
#define TICK_IOCMD_STOP     DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN, 0, 1, 0)

/*********************
 *      Buzzer
 *********************/

typedef struct
{
    uint32_t freq;
    uint32_t duration;
} Buzzer_Info_t;

/* clang-format on */

} /* namespace HAL */
#endif
