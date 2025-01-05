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
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#define LOG_SERIAL CONFIG_LOG_SERIAL

static uint8_t logLevel = HAL_LOG_LEVEL_INFO;

extern "C" {

void HAL_Log_Init()
{
    LOG_SERIAL.begin(115200);
    LOG_SERIAL.setTimeout(20);
}

void HAL_Log_SetLevel(uint8_t level)
{
    if (level > _HAL_LOG_LEVEL_LAST) {
        return;
    }

    logLevel = level;
}

void HAL_Log_PrintString(const char* str)
{
    LOG_SERIAL.print(str);
}

void HAL_Log(uint8_t level, const char* func, const char* fmt, ...)
{
    if (level < logLevel) {
        return;
    }

    static const char* prompt[_HAL_LOG_LEVEL_LAST] = {
        "TRACE", "INFO", "WARN", "ERROR"
    };

    char buffer[256];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    HAL_Log_Printf("[%" PRIu32 "][%s] %s: %s\r\n", HAL::GetTick(), prompt[level], func, buffer);
}

void HAL_Log_Printf(const char* fmt, ...)
{
    char buffer[256];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    LOG_SERIAL.printf("%s", buffer);
}
} /* extern "C" */
