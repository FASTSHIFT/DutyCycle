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

#define KVDB_GET(value) _kvdb.get(#value, &value, sizeof(value))
#define KVDB_SET(value) _kvdb.set(#value, &value, sizeof(value))

using namespace DataProc;

class DP_Ctrl {
public:
    DP_Ctrl(DataNode* node);

private:
    DataNode* _node;
    const DataNode* _nodeClock;
    const DataNode* _nodeGlobal;
    KVDB_Helper _kvdb;
    DeviceObject* _devMotor;

    bool _enablePrint;
    bool _enableClockMap;
    int _sweepValue;

private:
    int onEvent(DataNode::EventParam_t* param);
    int onNotify(const Ctrl_Info_t* info);
    void onTimer();
    void onClockEvent(const HAL::Clock_Info_t* info);
    void onGlobalEvent(const Global_Info_t* info);
    void setEnablePrint(bool enable);
    void setMotorValue(int value);
    uint32_t getTimestamp(int hour, int minute, int second);
    inline uint32_t getTimestamp(int hour)
    {
        return getTimestamp(hour, 0, 0);
    }
    int32_t timestampMap(int32_t x, int32_t hour_start, int32_t hour_end, int32_t min_out, int32_t max_out);
    int32_t valueMap(int32_t x, int32_t max_in, int32_t min_in, int32_t min_out, int32_t max_out);
};

DP_Ctrl::DP_Ctrl(DataNode* node)
    : _node(node)
    , _kvdb(node)
    , _enablePrint(false)
    , _enableClockMap(true)
{
    _devMotor = HAL::Manager()->getDevice("Motor");
    if (!_devMotor) {
        return;
    }

    _nodeClock = _node->subscribe("Clock");
    if (!_nodeClock) {
        return;
    }

    _nodeGlobal = _node->subscribe("Global");

    _node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_Ctrl*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_PUBLISH | DataNode::EVENT_NOTIFY | DataNode::EVENT_TIMER);
}

int DP_Ctrl::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {

    case DataNode::EVENT_PUBLISH:
        if (param->tran == _nodeGlobal) {
            onGlobalEvent((const Global_Info_t*)param->data_p);
        } else if (param->tran == _nodeClock) {
            onClockEvent((const HAL::Clock_Info_t*)param->data_p);
        }
        break;

    case DataNode::EVENT_NOTIFY:
        if (param->size != sizeof(Ctrl_Info_t)) {
            return DataNode::RES_SIZE_MISMATCH;
        }
        return onNotify((const Ctrl_Info_t*)param->data_p);

    case DataNode::EVENT_TIMER:
        onTimer();
        break;

    default:
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }

    return DataNode::RES_OK;
}

int DP_Ctrl::onNotify(const Ctrl_Info_t* info)
{
    switch (info->cmd) {
    case CTRL_CMD::SWEEP_TEST:
        _enableClockMap = false;
        _sweepValue = 0;
        _node->startTimer(100);
        break;
    case CTRL_CMD::ENABLE_PRINT:
        setEnablePrint(true);
        break;
    case CTRL_CMD::DISABLE_PRINT:
        setEnablePrint(false);
        break;
    case CTRL_CMD::SET_MOTOR_VALUE:
        _enableClockMap = false;
        setMotorValue(info->motorValue);
        break;
    case CTRL_CMD::SET_CLOCK_MAP:
        break;
    case CTRL_CMD::ENABLE_CLOCK_MAP:
        _enableClockMap = true;
        break;
    default:
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }
    
    return DataNode::RES_OK;
}

void DP_Ctrl::onTimer()
{
    int motorValue = 0;

    if (_sweepValue < 1000) {
        motorValue = _sweepValue;
    } else if (_sweepValue < 2000) {
        motorValue = valueMap(_sweepValue, 1000, 2000, 1000, 0);
    } else if (_sweepValue < 3000) {
        motorValue = valueMap(_sweepValue, 2000, 3000, 0, -1000);
    } else if (_sweepValue < 4000) {
        motorValue = valueMap(_sweepValue, 3000, 4000, -1000, 0);
    } else {
        HAL_LOG_INFO("Sweep test finished");
        _node->stopTimer();
    }

    setMotorValue(motorValue);

    _sweepValue += 10;
}

void DP_Ctrl::onGlobalEvent(const Global_Info_t* info)
{
    if (info->event == GLOBAL_EVENT::APP_STARTED) {
        KVDB_GET(_enablePrint);
    }
}

void DP_Ctrl::setEnablePrint(bool enable)
{
    _enablePrint = enable;
    KVDB_SET(_enablePrint);
}

#define MOTOR_VALUE_5AM 690
#define MOTOR_VALUE_7AM 520
#define MOTOR_VALUE_9AM 300
#define MOTOR_VALUE_12AM 0
#define MOTOR_VALUE_9PM -310
#define MOTOR_VALUE_12PM -490
#define MOTOR_VALUE_1AM -540
#define MOTOR_VALUE_5PM_DOWN -710

// #define MOTOR_VALUE_5AM 485
// #define MOTOR_VALUE_7AM 365
// #define MOTOR_VALUE_9AM 210
// #define MOTOR_VALUE_12AM 0
// #define MOTOR_VALUE_9PM -200
// #define MOTOR_VALUE_12PM -315
// #define MOTOR_VALUE_1AM -350
// #define MOTOR_VALUE_5PM_DOWN -470

void DP_Ctrl::onClockEvent(const HAL::Clock_Info_t* info)
{
    if (!_enableClockMap) {
        return;
    }

    uint32_t curTimestamp = getTimestamp(info->hour, info->minute, info->second);

    if (_enablePrint) {
        HAL_LOG_INFO("Current times: %04d-%02d-%02d %02d:%02d:%02d.%03d, timestamp: %d",
            info->year, info->month, info->day, info->hour, info->minute, info->second, info->millisecond, curTimestamp);
    }

    //    static int timeIndex = 0;
    //    static const int timeTable[] = { 5, 7, 9, 12, 21, 0, 1, 4 };
    //    if (timeIndex++ >= sizeof(timeTable) / sizeof(timeTable[0])) {
    //        timeIndex = 0;
    //    }
    //    curTimestamp = getTimestamp(timeTable[timeIndex], 0, 0);

    int motorValue = 0;

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

    setMotorValue(motorValue);
}

void DP_Ctrl::setMotorValue(int value)
{
    _devMotor->write(&value, sizeof(value));
}

uint32_t DP_Ctrl::getTimestamp(int hour, int minute, int second)
{
    return hour * 3600 + minute * 60 + second;
}

int32_t DP_Ctrl::timestampMap(int32_t x, int32_t hour_start, int32_t hour_end, int32_t min_out, int32_t max_out)
{
    int32_t min_in = getTimestamp(hour_start);
    int32_t max_in = getTimestamp(hour_end);
    return valueMap(x, min_in, max_in, min_out, max_out);
}

int32_t DP_Ctrl::valueMap(int32_t x, int32_t min_in, int32_t max_in, int32_t min_out, int32_t max_out)
{
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
