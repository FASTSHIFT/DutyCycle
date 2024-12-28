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
#ifndef __DATA_PROC_BUTTON_DEF_H
#define __DATA_PROC_BUTTON_DEF_H

#include <stdint.h>

namespace DataProc {

/* Button */

enum class BUTTON_ID {
    OK,
    UP,
    DOWN
};

enum class BUTTON_EVENT {
    NONE,
    PRESSED,
    PRESSING,
    LONG_PRESSED,
    LONG_PRESSED_REPEAT,
    LONG_PRESSED_RELEASED,
    RELEASED,
    CHANGED,
    CLICKED,
    SHORT_CLICKED,
    DOUBLE_CLICKED
};

typedef struct Button_Info {
    Button_Info()
        : id(BUTTON_ID::OK)
        , event(BUTTON_EVENT::NONE)

    {
    }
    BUTTON_ID id;
    BUTTON_EVENT event;
} Button_Info_t;

} // namespace DataProc

#endif // __DATA_PROC_BUTTON_DEF_H
