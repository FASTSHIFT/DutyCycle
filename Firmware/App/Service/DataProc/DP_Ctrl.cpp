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
#include "Utils/CommonMacro/CommonMacro.h"

#define KVDB_GET(value) _kvdb.get(#value, &value, sizeof(value))
#define KVDB_SET(value) _kvdb.set(#value, &value, sizeof(value))

#define MOTOR_VALUE_MIN -1000
#define MOTOR_VALUE_MAX 1000
#define MOTOR_VALUE_INVALID -32768

using namespace DataProc;

class DP_Ctrl {
public:
    DP_Ctrl(DataNode* node);

private:
    enum class DISPLAY_STATE {
        CLOCK_MAP,
        SWEEP_TEST,
        MOTOR_SET,
        BATTERY_USAGE,
    };

private:
    DataNode* _node;
    const DataNode* _nodeClock;
    const DataNode* _nodeGlobal;
    const DataNode* _nodeButton;
    KVDB_Helper _kvdb;
    DeviceObject* _devMotor;
    DeviceObject* _devBattery;

    int16_t _hourMotorMap[25];

    DISPLAY_STATE _displayState;
    int _sweepValue;

private:
    int onEvent(DataNode::EventParam_t* param);
    int onNotify(const Ctrl_Info_t* info);
    void onTimer();
    void onClockEvent(const HAL::Clock_Info_t* info);
    void onGlobalEvent(const Global_Info_t* info);
    void onButtonEvent(const Button_Info_t* info);
    int setClockMap(int hour, int value);
    int setMotorValue(int value);
    void listHourMotorMap();
    void sweepTest();
    void showBatteryUsage();

    int timestampToMotorValue(uint32_t timestamp);

    uint32_t getTimestamp(int hour, int minute = 0, int second = 0);
    int32_t timestampMap(int32_t x, int32_t hour_start, int32_t hour_end, int32_t min_out, int32_t max_out);
    int32_t valueMap(int32_t x, int32_t max_in, int32_t min_in, int32_t min_out, int32_t max_out);
};

DP_Ctrl::DP_Ctrl(DataNode* node)
    : _node(node)
    , _kvdb(node)
    , _displayState(DISPLAY_STATE::CLOCK_MAP)
{
    for (int i = 0; i < CM_ARRAY_SIZE(_hourMotorMap); i++) {
        _hourMotorMap[i] = MOTOR_VALUE_INVALID;
    }

    _devMotor = HAL::Manager()->getDevice("Motor");
    if (!_devMotor) {
        return;
    }

    _devBattery = HAL::Manager()->getDevice("Battery");

    _nodeClock = _node->subscribe("Clock");
    if (!_nodeClock) {
        return;
    }

    _nodeGlobal = _node->subscribe("Global");
    _nodeButton = _node->subscribe("Button");

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
        } else if (param->tran == _nodeButton) {
            onButtonEvent((const Button_Info_t*)param->data_p);
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
        sweepTest();
        break;

    case CTRL_CMD::SET_MOTOR_VALUE:
        _displayState = DISPLAY_STATE::MOTOR_SET;
        return setMotorValue(info->motorValue);

    case CTRL_CMD::SET_CLOCK_MAP:
        return setClockMap(info->hour, info->motorValue);

    case CTRL_CMD::ENABLE_CLOCK_MAP:
        _displayState = DISPLAY_STATE::CLOCK_MAP;
        break;

    case CTRL_CMD::SHOW_BATTERY_USAGE:
        showBatteryUsage();
        break;

    default:
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }

    return DataNode::RES_OK;
}

void DP_Ctrl::onTimer()
{
    if (_displayState != DISPLAY_STATE::SWEEP_TEST) {
        HAL_LOG_WARN("Sweep test interrupted");
        _node->stopTimer();
        return;
    }

    int motorValue = 0;

    if (_sweepValue < MOTOR_VALUE_MAX) {
        motorValue = _sweepValue;
    } else if (_sweepValue < MOTOR_VALUE_MAX * 2) {
        motorValue = valueMap(_sweepValue, MOTOR_VALUE_MAX, MOTOR_VALUE_MAX * 2, MOTOR_VALUE_MAX, 0);
    } else if (_sweepValue < MOTOR_VALUE_MAX * 3) {
        motorValue = valueMap(_sweepValue, MOTOR_VALUE_MAX * 2, MOTOR_VALUE_MAX * 3, 0, MOTOR_VALUE_MIN);
    } else if (_sweepValue < MOTOR_VALUE_MAX * 4) {
        motorValue = valueMap(_sweepValue, MOTOR_VALUE_MAX * 3, MOTOR_VALUE_MAX * 4, MOTOR_VALUE_MIN, 0);
    } else {
        HAL_LOG_INFO("Sweep test finished");
        _node->stopTimer();
    }

    setMotorValue(motorValue);

    _sweepValue += (MOTOR_VALUE_MAX / 100);
}

void DP_Ctrl::onGlobalEvent(const Global_Info_t* info)
{
    if (info->event == GLOBAL_EVENT::APP_STARTED) {
        KVDB_GET(_hourMotorMap);
        listHourMotorMap();
    }
}

int DP_Ctrl::setClockMap(int hour, int value)
{
    if (hour < 0 || hour >= CM_ARRAY_SIZE(_hourMotorMap)) {
        HAL_LOG_ERROR("Invalid hour: %d", hour);
        return DataNode::RES_PARAM_ERROR;
    }

    if ((value < MOTOR_VALUE_MIN || value > MOTOR_VALUE_MAX) && value != MOTOR_VALUE_INVALID) {
        HAL_LOG_ERROR("Invalid motor value: %d", value);
        return DataNode::RES_PARAM_ERROR;
    }

    HAL_LOG_INFO("H:%d -> M:%d", hour, value);

    _hourMotorMap[hour] = value;

    listHourMotorMap();

    return KVDB_SET(_hourMotorMap);
}

void DP_Ctrl::onClockEvent(const HAL::Clock_Info_t* info)
{
    if (_displayState != DISPLAY_STATE::CLOCK_MAP) {
        return;
    }

    uint32_t curTimestamp = getTimestamp(info->hour, info->minute, info->second);

    HAL_LOG_TRACE("Current times: %04d-%02d-%02d %02d:%02d:%02d.%03d, timestamp: %d",
        info->year, info->month, info->day, info->hour, info->minute, info->second, info->millisecond, curTimestamp);

    setMotorValue(timestampToMotorValue(curTimestamp));
}

void DP_Ctrl::onButtonEvent(const Button_Info_t* info)
{
    switch (info->event) {
    case BUTTON_EVENT::PRESSED:
        showBatteryUsage();
        break;

    case BUTTON_EVENT::RELEASED:
        _displayState = DISPLAY_STATE::CLOCK_MAP;
        break;

    default:
        break;
    }
}

int DP_Ctrl::setMotorValue(int value)
{
    HAL_LOG_TRACE("value: %d", value);

    if (value < MOTOR_VALUE_MIN || value > MOTOR_VALUE_MAX) {
        HAL_LOG_ERROR("Invalid motor value: %d", value);
        return DataNode::RES_PARAM_ERROR;
    }

    return _devMotor->write(&value, sizeof(value)) == sizeof(value) ? DataNode::RES_OK : DataNode::RES_PARAM_ERROR;
}

void DP_Ctrl::listHourMotorMap()
{
    for (int i = 0; i < CM_ARRAY_SIZE(_hourMotorMap); i++) {
        if (_hourMotorMap[i] == MOTOR_VALUE_INVALID) {
            continue;
        }
        HAL_LOG_INFO("H:%d -> M:%d", i, _hourMotorMap[i]);
    }
}

void DP_Ctrl::sweepTest()
{
    _displayState = DISPLAY_STATE::SWEEP_TEST;
    _sweepValue = 0;
    _node->startTimer(100);
}

void DP_Ctrl::showBatteryUsage()
{
    if (!_devBattery) {
        HAL_LOG_WARN("No battery device found");
        return;
    }

    _displayState = DISPLAY_STATE::BATTERY_USAGE;

    if (_devBattery->ioctl(BATTERY_IOCMD_WAKEUP) != DeviceObject::RES_OK) {
        HAL_LOG_ERROR("Failed to wakeup battery device");
        return;
    }

    HAL::Battery_Info_t info;
    if (_devBattery->read(&info, sizeof(info)) == sizeof(info)) {
        HAL_LOG_INFO("voltage: %dmV, level: %d%%", info.voltage, info.level);

        auto timestamp_0_0_0 = getTimestamp(0, 0, 0);
        auto timestamp_5_0_0 = getTimestamp(5, 0, 0);
        auto timestamp_23_59_59 = getTimestamp(23, 59, 59);

        uint32_t timestamp = 0;
        static const uint32_t demarcationPct = timestamp_5_0_0 * 100 / timestamp_23_59_59;

        if (info.level >= demarcationPct) {
            timestamp = valueMap(info.level, 100, demarcationPct, timestamp_5_0_0, timestamp_23_59_59);
        } else {
            timestamp = valueMap(info.level, demarcationPct, 0, timestamp_0_0_0, timestamp_5_0_0);
        }

        setMotorValue(timestampToMotorValue(timestamp));

    } else {
        HAL_LOG_ERROR("Failed to read battery info");
        return;
    }

    _devBattery->ioctl(BATTERY_IOCMD_SLEEP);
}

int DP_Ctrl::timestampToMotorValue(uint32_t timestamp)
{
    int motorValue = 0;

    // 42L6
    // #define MOTOR_VALUE_5AM 690
    // #define MOTOR_VALUE_7AM 520
    // #define MOTOR_VALUE_9AM 300
    // #define MOTOR_VALUE_12AM 0
    // #define MOTOR_VALUE_9PM -310
    // #define MOTOR_VALUE_12PM -490
    // #define MOTOR_VALUE_1AM -540
    // #define MOTOR_VALUE_5PM_DOWN -710

    // 6L2
    // #define MOTOR_VALUE_5AM 485
    // #define MOTOR_VALUE_7AM 365
    // #define MOTOR_VALUE_9AM 210
    // #define MOTOR_VALUE_12AM 0
    // #define MOTOR_VALUE_9PM -200
    // #define MOTOR_VALUE_12PM -315
    // #define MOTOR_VALUE_1AM -350
    // #define MOTOR_VALUE_5PM_DOWN -470

#define MOTOR_VALUE_5AM _hourMotorMap[5]
#define MOTOR_VALUE_7AM _hourMotorMap[7]
#define MOTOR_VALUE_9AM _hourMotorMap[9]
#define MOTOR_VALUE_12AM _hourMotorMap[12]
#define MOTOR_VALUE_9PM _hourMotorMap[21]
#define MOTOR_VALUE_12PM _hourMotorMap[0]
#define MOTOR_VALUE_1AM _hourMotorMap[1]
#define MOTOR_VALUE_5PM_DOWN _hourMotorMap[24]

    if (timestamp >= getTimestamp(5, 0, 0) && timestamp < getTimestamp(7, 0, 0)) {
        motorValue = timestampMap(timestamp, 5, 7, MOTOR_VALUE_5AM, MOTOR_VALUE_7AM);
    } else if (timestamp >= getTimestamp(7, 0, 0) && timestamp < getTimestamp(9, 0, 0)) {
        motorValue = timestampMap(timestamp, 7, 9, MOTOR_VALUE_7AM, MOTOR_VALUE_9AM);
    } else if (timestamp >= getTimestamp(9, 0, 0) && timestamp < getTimestamp(12, 0, 0)) {
        motorValue = timestampMap(timestamp, 9, 12, MOTOR_VALUE_9AM, MOTOR_VALUE_12AM);
    } else if (timestamp >= getTimestamp(12, 0, 0) && timestamp < getTimestamp(21, 0, 0)) {
        motorValue = timestampMap(timestamp, 12, 21, MOTOR_VALUE_12AM, MOTOR_VALUE_9PM);
    } else if (timestamp >= getTimestamp(21, 0, 0) && timestamp < getTimestamp(24, 0, 0)) {
        motorValue = timestampMap(timestamp, 21, 24, MOTOR_VALUE_9PM, MOTOR_VALUE_12PM);
    } else if (timestamp >= getTimestamp(0, 0, 0) && timestamp < getTimestamp(1, 0, 0)) {
        motorValue = timestampMap(timestamp, 0, 1, MOTOR_VALUE_12PM, MOTOR_VALUE_1AM);
    } else if (timestamp >= getTimestamp(1, 0, 0) && timestamp < getTimestamp(5, 0, 0)) {
        motorValue = timestampMap(timestamp, 1, 5, MOTOR_VALUE_1AM, MOTOR_VALUE_5PM_DOWN);
    }

    return motorValue;
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
