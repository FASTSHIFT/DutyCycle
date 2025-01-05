/*
 * MIT License
 * Copyright (c) 2021 - 2025 _VIFEXTech
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
#ifndef __DATA_PROC_ALARM_DEF_H
#define __DATA_PROC_ALARM_DEF_H

#include <stdint.h>

namespace DataProc {

enum class ALARM_CMD {
    NONE,
    SET,
    SAVE,
    LIST,
    ENABLE_HOURLY_ALARM,
    DISABLE_HOURLY_ALARM,
    SET_HOURLY_ALARM_START,
    SET_HOURLY_ALARM_END,
    PLAY_ALARM_MUSIC,
};

typedef struct Alarm_Info {
    Alarm_Info()
        : cmd(ALARM_CMD::NONE)
        , id(-1)
        , hour(-1)
        , minute(0)
        , musicID(0)
    {
    }
    ALARM_CMD cmd;
    int id;
    int hour;
    int minute;
    int musicID;
} Alarm_Info_t;

} // namespace DataProc

#endif // __DATA_PROC_ALARM_DEF_H
