/*
 * MIT License
 * Copyright (c) 2023 - 2024 _VIFEXTech
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
#include "HAL.h"
#include "rtc.h"

namespace HAL {

class Clock : private DeviceObject {
public:
    Clock(const char* name)
        : DeviceObject(name)
    {
    }

    static void getInfo(HAL::Clock_Info_t* info);

private:
    virtual int onInit();
    virtual int onRead(void* buffer, size_t size);
    virtual int onIoctl(DeviceObject::IO_Cmd_t cmd, void* data);
    int calibrate(const HAL::Clock_Info_t* info);
    const char* convWeekString(uint8_t week);
};

int Clock::onInit()
{
    RTC_Init();

    HAL::Clock_Info_t info;
    getInfo(&info);

    HAL_LOG_INFO(
        "Time: %04d-%02d-%02d %s %02d:%02d:%02d.%d",
        info.year,
        info.month,
        info.day,
        convWeekString(info.week),
        info.hour,
        info.minute,
        info.second,
        info.millisecond);

    return DeviceObject::RES_OK;
}

int Clock::onRead(void* buffer, size_t size)
{
    if (size != sizeof(HAL::Clock_Info_t)) {
        return DeviceObject::RES_PARAM_ERROR;
    }
    getInfo((HAL::Clock_Info_t*)buffer);
    return sizeof(HAL::Clock_Info_t);
}

int Clock::onIoctl(DeviceObject::IO_Cmd_t cmd, void* data)
{
    switch (cmd.full) {
    case CLOCK_IOCMD_CALIBRATE:
        return calibrate((HAL::Clock_Info_t*)data);

    default:
        return DeviceObject::RES_UNSUPPORT;
    }

    return DeviceObject::RES_OK;
}

void Clock::getInfo(HAL::Clock_Info_t* info)
{
    memset(info, 0, sizeof(HAL::Clock_Info_t));

    RTC_Calendar_TypeDef calendar;
    RTC_GetCalendar(&calendar);
    info->year = calendar.year;
    info->month = calendar.month;
    info->day = calendar.day;
    info->week = calendar.week % 7;
    info->hour = calendar.hour;
    info->minute = calendar.min;
    info->second = calendar.sec;

    const uint32_t sub_second_max = ERTC->div_bit.divb;
    info->millisecond = (sub_second_max - ertc_sub_second_get()) * 1000 / sub_second_max;
}

int Clock::calibrate(const HAL::Clock_Info_t* info)
{
    if (!RTC_SetTime(
            info->year,
            info->month,
            info->day,
            info->hour,
            info->minute,
            info->second)) {
        return DeviceObject::RES_UNKNOWN;
    }

    if (!info->calPeriodSec) {
        return DeviceObject::RES_OK;
    }

    return RTC_SetCalibration(info->calPeriodSec, info->calOffsetClk) ? DeviceObject::RES_OK : DeviceObject::RES_UNKNOWN;
}

const char* Clock::convWeekString(uint8_t week)
{
    static const char* week_str[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

    return week < 7 ? week_str[week] : "ERR";
}

} /* namespace HAL */

uint32_t HAL::GetTick()
{
    HAL::Clock_Info_t info;
    HAL::Clock::getInfo(&info);
    const uint64_t seconds = (info.year * 365 + info.month * 30 + info.day) * 24 * 60 * 60
        + info.hour * 60 * 60 + info.minute * 60
        + info.second;
    return (uint32_t)(seconds * 1000 + info.millisecond);
}

DEVICE_OBJECT_MAKE(Clock);
