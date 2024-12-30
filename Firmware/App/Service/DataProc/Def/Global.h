/*
 * MIT License
 * Copyright (c) 2021 - 2024 _VIFEXTech
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
#ifndef __DATA_PROC_GLOBAL_DEF_H
#define __DATA_PROC_GLOBAL_DEF_H

#include <cstdint>

namespace DataProc {

enum class GLOBAL_EVENT {
    NONE,
    DATA_PROC_INIT_FINISHED,
    APP_STARTED,
    APP_STOPPED,
    APP_RUN_LOOP_BEGIN,
    APP_RUN_LOOP_END,
};

typedef struct Global_Info {
    Global_Info()
        : event(GLOBAL_EVENT::NONE)
    {
    }
    GLOBAL_EVENT event;
    void * param;
} Global_Info_t;

} // namespace DataProc

#endif // __DATA_PROC_GLOBAL_DEF_H