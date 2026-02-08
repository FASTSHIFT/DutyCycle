/*
 * MIT License
 * Copyright (c) 2026 _VIFEXTech
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

#include "Service/HAL/HAL.h"
#include "Utils/easing/easing.h"

#ifndef __DATA_PROC_MOTOR_CTRL_H
#define __DATA_PROC_MOTOR_CTRL_H

class DataNode;

namespace DataProc {

class MotorCtrl {
public:
    enum class UNIT {
        NONE,
        HOUR,
        HOUR_COS_PHI,
        MINUTE,
        SECOND,
    };

    enum class DISPLAY_STATE {
        CLOCK_MAP,
        SWEEP_TEST,
        MOTOR_SET,
        BATTERY_USAGE,
    };

    typedef void (*Callback_t)(void* ctx, int16_t value);

public:
    /* For KVDB_Helper directly access */
    int16_t _valueMap[25];
    UNIT _unit;

public:
    MotorCtrl();

    void setUnit(UNIT unit) { _unit = unit; }
    void setID(uint8_t id) { _id = id; }
    void setNode(DataNode* node) { _node = node; }
    void setDevice(DeviceObject* dev) { _dev = dev; }
    void setDisplayState(DISPLAY_STATE state) { _displayState = state; }

    int setValueMap(uint8_t index, int16_t value);
    void setMotorValue(int value, bool immediate = false);
    void update(const HAL::Clock_Info_t* info);

    void listMap();
    void sweepTest();
    void showLevel(int16_t level);
    bool timerHandler();

private:
    DataNode* _node;
    DeviceObject* _dev;
    int8_t _sweepValueIndex;
    uint8_t _id : 4;
    DISPLAY_STATE _displayState : 4;
    easing_t _easing;

private:
    int getMotorValueRaw();
    int setMotorValueRaw(int value);
    void onMotorFinished();

    int timestampToMotorValue(int timestamp);
    int32_t timestampMap(int32_t x, int32_t hour_start, int32_t hour_end, int32_t min_out, int32_t max_out);
    int32_t timestampMap(int32_t x, int32_t hour_start, int32_t hour_end);
    int32_t valueMap(int32_t x, int32_t max_in, int32_t min_in, int32_t min_out, int32_t max_out);
};

} // namespace DataProc

#endif // __DATA_PROC_MOTOR_CTRL_H
