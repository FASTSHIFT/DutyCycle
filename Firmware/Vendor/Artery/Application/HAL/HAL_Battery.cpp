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

#define BATTERY_FULL_VOLTAGE 4100 /* mv */
#define BATTERY_LOW_VOLTAGE 3100 /* mv */

namespace HAL {

class Battery : private DeviceObject {
public:
    Battery(const char* name)
        : DeviceObject(name)
    {
    }

private:
    virtual int onInit();
    virtual int onRead(void* buffer, size_t size);
    virtual int onIoctl(DeviceObject::IO_Cmd_t cmd, void* data);
};

int Battery::onInit()
{
    pinMode(CONFIG_BATT_DET_PIN, INPUT_ANALOG);

    Battery_Info_t info;
    onRead(&info, sizeof(info));
    HAL_LOG_INFO("voltage: %dmV, level: %d%%", info.voltage, info.level);

    return DeviceObject::RES_OK;
}

int Battery::onRead(void* buffer, size_t size)
{
    if (size != sizeof(Battery_Info_t)) {
        return DeviceObject::RES_PARAM_ERROR;
    }

    uint16_t voltage = analogRead(CONFIG_BATT_DET_PIN) * 2 * 1000 / 4095;

    auto info = (Battery_Info_t*)buffer;
    info->voltage = voltage;
    CM_VALUE_LIMIT(voltage, BATTERY_LOW_VOLTAGE, BATTERY_FULL_VOLTAGE);
    info->level = CM_VALUE_MAP(voltage, BATTERY_LOW_VOLTAGE, BATTERY_FULL_VOLTAGE, 0, 100);
    info->isReady = true;
    info->isCharging = false;

    return sizeof(Battery_Info_t);
}

int Battery::onIoctl(DeviceObject::IO_Cmd_t cmd, void* data)
{
    switch (cmd.full) {
    case BATTERY_IOCMD_SLEEP:
        break;
    default:
        return DeviceObject::RES_UNSUPPORT;
    }
    return DeviceObject::RES_OK;
}

} /* namespace HAL */
DEVICE_OBJECT_MAKE(Battery);
