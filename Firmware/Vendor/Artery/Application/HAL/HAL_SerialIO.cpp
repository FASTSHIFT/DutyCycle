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

class SerialIO : private DeviceObject {
public:
    SerialIO(const char* name)
        : DeviceObject(name)
    {
    }

private:
    virtual int onInit();
    virtual int onRead(void* buffer, size_t size);
    virtual int onWrite(const void* buffer, size_t size);
};

int SerialIO::onInit()
{
    return DeviceObject::RES_OK;
}

int SerialIO::onRead(void* buffer, size_t size)
{
    uint8_t* data = (uint8_t*)buffer;
    int readCnt = 0;
    while (size--) {
        if (!CONFIG_LOG_SERIAL.available()) {
            break;
        }

        *data++ = CONFIG_LOG_SERIAL.read();
        readCnt++;
    }

    return readCnt;
}

int SerialIO::onWrite(const void* buffer, size_t size)
{
    CONFIG_LOG_SERIAL.write((const uint8_t*)buffer, size);
    return size;
}

} /* namespace HAL */

DEVICE_OBJECT_MAKE(SerialIO);
