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
#ifndef __DATA_PROC_AUDIO_DEF_H
#define __DATA_PROC_AUDIO_DEF_H

#include <stdint.h>

namespace DataProc {

typedef struct Audio_Squence {
    Audio_Squence(uint32_t freq = 0, uint16_t dur = 0, uint32_t time = 0)
        : frequency(freq)
        , duration(dur)
        , time(time)
    {
    }
    uint32_t frequency;
    uint32_t duration;
    uint32_t time;
} Audio_Squence_t;

typedef struct Audio_Info {
    Audio_Info()
        : squence(nullptr)
        , length(0)
    {
    }
    const Audio_Squence_t* squence;
    uint32_t length;
} Audio_Info_t;

} // namespace DataProc

#endif // __DATA_PROC_AUDIO_DEF_H
