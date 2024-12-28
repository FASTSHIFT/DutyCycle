/*
 * MIT License
 * Copyright (c) 2023 _VIFEXTech
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
#include "wdg.h"

namespace HAL {

class WatchDog : private DeviceObject {
public:
    WatchDog(const char* name)
        : DeviceObject(name)
    {
        _instance = this;
    }

private:
    static WatchDog* _instance;

private:
    virtual int onInit();
    virtual int onIoctl(DeviceObject::IO_Cmd_t cmd, void* data);
};

WatchDog* WatchDog::_instance = nullptr;

int WatchDog::onInit()
{
    return DeviceObject::RES_OK;
}

int WatchDog::onIoctl(DeviceObject::IO_Cmd_t cmd, void* data)
{
    int retval = DeviceObject::RES_OK;
    switch(cmd.full)
    {
    case WATCHDOG_IOCMD_SET_TIMEOUT:
        retval = WDG_SetTimeout(*(int*)data);
        break;
    case WATCHDOG_IOCMD_ENABLE:
        WDG_SetEnable();
        break;
    case WATCHDOG_IOCMD_KEEP_ALIVE:
        WDG_ReloadCounter();
        break;
    default:
        return DeviceObject::RES_PARAM_ERROR;
    }
    return retval;
}

} /* namespace HAL */

DEVICE_OBJECT_MAKE(WatchDog);
