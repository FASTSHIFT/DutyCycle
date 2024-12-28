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

#if CONFIG_MEMORY_STACK_INFO

#include "StackInfo/StackInfo.h"

static void Memory_DumpStackInfo()
{
    HAL_LOG_INFO(
        "Stack: %d%% used (total: %d, free: %d)",
        (int)(StackInfo_GetMaxUtilization() * 100),
        StackInfo_GetTotalSize(),
        StackInfo_GetMinFreeSize());
}
#endif

#if CONFIG_MEMORY_HEAP_INFO

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

static int Memory_HeapPrint(void* param, char const* format, ...)
{
    static char printf_buff[64];

    va_list args;
    va_start(args, format);

    int ret_status = vsnprintf(printf_buff, sizeof(printf_buff), format, args);

    va_end(args);

    HAL_LOG_INFO("Heap: %s", printf_buff);

    return ret_status;
}

static void Memory_DumpHeapInfo()
{
    int size = 0;
    __heapstats((__heapprt)Memory_HeapPrint, &size);
}
#endif

void HAL_MemoryDumpInfo()
{
#if CONFIG_MEMORY_STACK_INFO
    Memory_DumpStackInfo();
#endif

#if CONFIG_MEMORY_HEAP_INFO
    Memory_DumpHeapInfo();
#endif
}
