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
#ifndef __DATA_PROC_POWER_DEF_H
#define __DATA_PROC_POWER_DEF_H

#include <stdint.h>

namespace DataProc {

enum class POWER_CMD {
    NONE,
    UPDATE_INFO,
    SHUTDOWN,
    REBOOT,
    LOCK_WAKEUP,
    UNLOCK_WAKEUP,
    KICK_WAKUP,
    SET_AUTO_SHUTDOWN_TIME,
};

typedef struct Power_Info {
    Power_Info()
        : cmd(POWER_CMD::NONE)
        , isReady(false)
        , isCharging(false)
        , isBatteryLow(false)
        , level(0)
        , voltage(0)
        , autoShutdownTime(0)
        , batteryUseTime(0)
        , uptime(0)
    {
    }
    POWER_CMD cmd;
    bool isReady;
    bool isCharging;
    bool isBatteryLow;
    uint8_t level;
    uint16_t voltage;
    int autoShutdownTime;
    uint64_t batteryUseTime;
    uint64_t uptime;
} Power_Info_t;

} // namespace DataProc

#endif // __DATA_PROC_POWER_DEF_H
