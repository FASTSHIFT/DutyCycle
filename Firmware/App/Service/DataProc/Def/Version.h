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
#ifndef __DATA_PROC_VERSION_DEF_H
#define __DATA_PROC_VERSION_DEF_H

#include <cstdint>

namespace DataProc {

typedef struct Version_Info {
    Version_Info()
        : name(nullptr)
        , software(nullptr)
        , hardware(nullptr)
        , author(nullptr)
        , website(nullptr)
        , compiler(nullptr)
        , buildDate(nullptr)
        , buildTime(nullptr)
    {
    }
    const char* name;
    const char* software;
    const char* hardware;
    const char* author;
    const char* website;
    const char* compiler;
    const char* buildDate;
    const char* buildTime;
} Version_Info_t;

} // namespace DataProc

#endif // __DATA_PROC_VERSION_DEF_H
