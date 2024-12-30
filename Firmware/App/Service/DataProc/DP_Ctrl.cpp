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
#include "DataProc.h"
#include "Service/HAL/HAL.h"
#include "Service/HAL/HAL_Log.h"

using namespace DataProc;

class DP_Ctrl {
public:
    DP_Ctrl(DataNode* node);

private:
    DataNode* _node;
    const DataNode* _nodeColok;
    DeviceObject* _devMotor;

private:
    int onEvent(DataNode::EventParam_t* param);
    void onClockEvent(const HAL::Clock_Info_t* info);
    void motorWrite(int value);
    uint32_t getTimestamp(int hour, int minute, int second);
    int32_t timestampMap(int32_t x, int32_t hour_start, int32_t hour_end, int32_t min_out, int32_t max_out);
};

DP_Ctrl::DP_Ctrl(DataNode* node)
    : _node(node)
{
    _devMotor = HAL::Manager()->getDevice("Motor");
    if (!_devMotor) {
        return;
    }

    _nodeColok = _node->subscribe("Clock");
    if (!_nodeColok) {
        return;
    }

    _node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_Ctrl*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_PUBLISH);
}

int DP_Ctrl::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {

    case DataNode::EVENT_PUBLISH:
        if (param->tran == _nodeColok) {
            onClockEvent((const HAL::Clock_Info_t*)param->data_p);
        }
        break;

    default:
        break;
    }

    return DataNode::RES_OK;
}

#define MOTOR_VALUE_5AM 690
#define MOTOR_VALUE_7AM 520
#define MOTOR_VALUE_9AM 300
#define MOTOR_VALUE_12AM 0
#define MOTOR_VALUE_9PM -310
#define MOTOR_VALUE_12PM -490
#define MOTOR_VALUE_1AM -540
#define MOTOR_VALUE_5PM_DOWN -710

void DP_Ctrl::onClockEvent(const HAL::Clock_Info_t* info)
{
    uint32_t curTimestamp = getTimestamp(info->hour, info->minute, info->second);
//    HAL_LOG_INFO("Current times: %04d-%02d-%02d %02d:%02d:%02d.%03d, timestamp: %d",
//        info->year, info->month, info->day, info->hour, info->minute, info->second, info->millisecond, curTimestamp);

    //    static int timeIndex = 0;
    //    static const int timeTable[] = { 5, 7, 9, 12, 21, 0, 1, 4 };
    //    if (timeIndex++ >= sizeof(timeTable) / sizeof(timeTable[0])) {
    //        timeIndex = 0;
    //    }
    //    curTimestamp = getTimestamp(timeTable[timeIndex], 0, 0);

    static int motorValue = 0;

    if (curTimestamp >= getTimestamp(5, 0, 0) && curTimestamp < getTimestamp(7, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 5, 7, MOTOR_VALUE_5AM, MOTOR_VALUE_7AM);
    } else if (curTimestamp >= getTimestamp(7, 0, 0) && curTimestamp < getTimestamp(9, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 7, 9, MOTOR_VALUE_7AM, MOTOR_VALUE_9AM);
    } else if (curTimestamp >= getTimestamp(9, 0, 0) && curTimestamp < getTimestamp(12, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 9, 12, MOTOR_VALUE_9AM, MOTOR_VALUE_12AM);
    } else if (curTimestamp >= getTimestamp(12, 0, 0) && curTimestamp < getTimestamp(21, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 12, 21, MOTOR_VALUE_12AM, MOTOR_VALUE_9PM);
    } else if (curTimestamp >= getTimestamp(21, 0, 0) && curTimestamp < getTimestamp(24, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 21, 24, MOTOR_VALUE_9PM, MOTOR_VALUE_12PM);
    } else if (curTimestamp >= getTimestamp(0, 0, 0) && curTimestamp < getTimestamp(1, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 0, 1, MOTOR_VALUE_12PM, MOTOR_VALUE_1AM);
    } else if (curTimestamp >= getTimestamp(1, 0, 0) && curTimestamp < getTimestamp(5, 0, 0)) {
        motorValue = timestampMap(curTimestamp, 1, 5, MOTOR_VALUE_1AM, MOTOR_VALUE_5PM_DOWN);
    }

    motorWrite(motorValue);
}

void DP_Ctrl::motorWrite(int value)
{
    _devMotor->write(&value, sizeof(value));
}

uint32_t DP_Ctrl::getTimestamp(int hour, int minute, int second)
{
    return hour * 3600 + minute * 60 + second;
}

int32_t DP_Ctrl::timestampMap(int32_t x, int32_t hour_start, int32_t hour_end, int32_t min_out, int32_t max_out)
{
    int32_t min_in = getTimestamp(hour_start, 0, 0);
    int32_t max_in = getTimestamp(hour_end, 0, 0);

    if (max_in >= min_in && x >= max_in)
        return max_out;
    if (max_in >= min_in && x <= min_in)
        return min_out;

    if (max_in <= min_in && x <= max_in)
        return max_out;
    if (max_in <= min_in && x >= min_in)
        return min_out;

    /**
     * The equation should be:
     *   ((x - min_in) * delta_out) / delta in) + min_out
     * To avoid rounding error reorder the operations:
     *   (x - min_in) * (delta_out / delta_min) + min_out
     */

    int32_t delta_in = max_in - min_in;
    int32_t delta_out = max_out - min_out;

    return ((x - min_in) * delta_out) / delta_in + min_out;
}

DATA_PROC_DESCRIPTOR_DEF(Ctrl)
