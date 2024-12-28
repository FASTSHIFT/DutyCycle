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

namespace HAL {

class Button : private DeviceObject {
public:
    Button(const char* name)
        : DeviceObject(name)
    {
    }

private:
    virtual int onInit();
    virtual int onRead(void* buffer, size_t size);
    void getInfo(HAL::Button_Info_t* info);
};

int Button::onInit()
{
    pinMode(CONFIG_BUTTON_SEL_PIN, INPUT_PULLUP);
    return DeviceObject::RES_OK;
}

int Button::onRead(void* buffer, size_t size)
{
    if (size != sizeof(HAL::Button_Info_t)) {
        return DeviceObject::RES_PARAM_ERROR;
    }

    getInfo((HAL::Button_Info_t*)buffer);
    return sizeof(HAL::Button_Info_t);
}

void Button::getInfo(HAL::Button_Info_t* info)
{
    info->value = 0;

    info->key.ok = !digitalRead(CONFIG_BUTTON_SEL_PIN);
}

} /* namespace HAL */

DEVICE_OBJECT_MAKE(Button);
