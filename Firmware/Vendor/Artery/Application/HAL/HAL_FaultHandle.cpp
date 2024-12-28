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
#include "HAL.h"
#include "Service/HAL/HAL_Assert.h"
#include "Version.h"
#include <stdarg.h>
#include <stdio.h>

namespace HAL {

class FaultHandle : private DeviceObject {
public:
    FaultHandle(const char* name)
        : DeviceObject(name)
    {
    }

private:
    virtual int onInit();
};

int FaultHandle::onInit()
{
    return DeviceObject::RES_OK;
}

} /* namespace HAL */

DEVICE_OBJECT_MAKE(FaultHandle);

extern "C" {

void cmb_printf(const char* __restrict __format, ...)
{
    char printf_buff[256];

    va_list args;
    va_start(args, __format);
    vsnprintf(printf_buff, sizeof(printf_buff), __format, args);
    va_end(args);

    HAL_Log_PrintString(printf_buff);
}

void HAL_Assert(const char* file, int line, const char* func, const char* expr)
{
    HAL_LOG_ERROR("Assert: %s:%d %s %s", file, line, func, expr);
    HAL_Panic();
}

void HAL_Panic(void)
{
}

} /* extern "C" */
