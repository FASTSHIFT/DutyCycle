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
#include "cm_backtrace/cm_backtrace.h"
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
    cm_backtrace_init(
        VERSION_FIRMWARE_NAME,
        VERSION_HARDWARE,
        VERSION_SOFTWARE " " __DATE__ " " __TIME__);
    return DeviceObject::RES_OK;
}

} /* namespace HAL */

DEVICE_OBJECT_MAKE(FaultHandle);

extern "C" {

/* clang-format off */
    
#ifdef __CC_ARM
    __asm void HardFault_Handler()
    {
        extern HAL_Panic
        extern cm_backtrace_fault

        mov r0, lr
        mov r1, sp
        bl cm_backtrace_fault
        bl HAL_Panic
fault_loop
        b fault_loop
    }
#elif __GNUC__
    [[noreturn]] void HardFault_Handler()
    {
        __asm volatile ("MOV     r0, lr             ");
        __asm volatile ("MOV     r1, sp             ");
        __asm volatile ("BL      cm_backtrace_fault ");
        __asm volatile ("BL      HAL_Panic ");

fault_loop:
        goto fault_loop;
    }
#else
#  warning "Unsupported platforms"
#endif

/* clang-format on */

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
    HAL_LOG_ERROR("FXXK PANIC !!!");
    HAL_LOG_ERROR("Firmware: " VERSION_FIRMWARE_NAME);
    HAL_LOG_ERROR("Software: " VERSION_SOFTWARE);
    HAL_LOG_ERROR("Hardware: " VERSION_HARDWARE);
    HAL_LOG_ERROR("Build Time: " __DATE__ " " __TIME__);
    NVIC_SystemReset();
}

} /* extern "C" */
