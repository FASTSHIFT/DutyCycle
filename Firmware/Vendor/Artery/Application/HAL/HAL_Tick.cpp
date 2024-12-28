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
#include <Arduino.h>

#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFF
#endif

namespace HAL {

class Tick : private DeviceObject {
public:
    Tick(const char* name)
        : DeviceObject(name)
    {
    }

private:
    virtual int onInit();
};

int Tick::onInit()
{
    return DeviceObject::RES_OK;
}


} // namespace HAL

uint32_t HAL::GetTick()
{
    return millis();
}

uint32_t HAL::GetTickElaps(uint32_t prevTick)
{
    uint32_t actTick = GetTick();

    /*If there is no overflow in sys_time simple subtract*/
    if (actTick >= prevTick) {
        prevTick = actTick - prevTick;
    } else {
        prevTick = UINT32_MAX - prevTick + 1;
        prevTick += actTick;
    }

    return prevTick;
}

DEVICE_OBJECT_MAKE(Tick);
