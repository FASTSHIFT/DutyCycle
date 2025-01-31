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
#include "Utils/easing/easing.h"

#define KVDB_GET(value) _kvdb.get(#value, &value, sizeof(value))
#define KVDB_SET(value) _kvdb.set(#value, &value, sizeof(value))

#define MOTOR_VALUE_MIN -1000
#define MOTOR_VALUE_MAX 1000
#define MOTOR_VALUE_INVALID -32768
#define MOTOR_TIMER_PERIOD 60
#define MOTOR_ANIM_SPEED_FACTOR 0.15f

// 42L6-cos-phi
// #define MOTOR_VALUE_H5 690
// #define MOTOR_VALUE_H7 520
// #define MOTOR_VALUE_H9 300
// #define MOTOR_VALUE_H12 0
// #define MOTOR_VALUE_H21 -310
// #define MOTOR_VALUE_H0 -490
// #define MOTOR_VALUE_H1 -540
// #define MOTOR_VALUE_H5_DOWN -710

// 6L2-cos-phi
// #define MOTOR_VALUE_H5 485
// #define MOTOR_VALUE_H7 365
// #define MOTOR_VALUE_H9 210
// #define MOTOR_VALUE_H12 0
// #define MOTOR_VALUE_H21 -200
// #define MOTOR_VALUE_H0 -315
// #define MOTOR_VALUE_H1 -350
// #define MOTOR_VALUE_H5_DOWN -470

// 42L6-linear-voltage
// #define MOTOR_VALUE_H0 0
// #define MOTOR_VALUE_H5 160
// #define MOTOR_VALUE_H10 305
// #define MOTOR_VALUE_H15 450
// #define MOTOR_VALUE_H20 595
// #define MOTOR_VALUE_H24 712

#define MOTOR_VALUE_H0 _hourMotorMap[0]
#define MOTOR_VALUE_H1 _hourMotorMap[1]
#define MOTOR_VALUE_H2 _hourMotorMap[2]
#define MOTOR_VALUE_H3 _hourMotorMap[3]
#define MOTOR_VALUE_H4 _hourMotorMap[4]
#define MOTOR_VALUE_H5 _hourMotorMap[5]
#define MOTOR_VALUE_H6 _hourMotorMap[6]
#define MOTOR_VALUE_H7 _hourMotorMap[7]
#define MOTOR_VALUE_H8 _hourMotorMap[8]
#define MOTOR_VALUE_H9 _hourMotorMap[9]
#define MOTOR_VALUE_H10 _hourMotorMap[10]
#define MOTOR_VALUE_H11 _hourMotorMap[11]
#define MOTOR_VALUE_H12 _hourMotorMap[12]
#define MOTOR_VALUE_H13 _hourMotorMap[13]
#define MOTOR_VALUE_H14 _hourMotorMap[14]
#define MOTOR_VALUE_H15 _hourMotorMap[15]
#define MOTOR_VALUE_H16 _hourMotorMap[16]
#define MOTOR_VALUE_H17 _hourMotorMap[17]
#define MOTOR_VALUE_H18 _hourMotorMap[18]
#define MOTOR_VALUE_H19 _hourMotorMap[19]
#define MOTOR_VALUE_H20 _hourMotorMap[20]
#define MOTOR_VALUE_H21 _hourMotorMap[21]
#define MOTOR_VALUE_H22 _hourMotorMap[22]
#define MOTOR_VALUE_H23 _hourMotorMap[23]
#define MOTOR_VALUE_H24 _hourMotorMap[24]
#define MOTOR_VALUE_H5_DOWN _hourMotorMap[24]

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
    int8_t _sweepValueIndex;
    DISPLAY_STATE _displayState : 4;
    CTRL_DISPLAY_MODE _displayMode;
    easing_t _easing;

private:
    int onEvent(DataNode::EventParam_t* param);
    int onNotify(const Ctrl_Info_t* info);
    void onTimer();
    void onClockEvent(const HAL::Clock_Info_t* info);
    void onGlobalEvent(const Global_Info_t* info);
    void onButtonEvent(const Button_Info_t* info);
    int setClockMap(int hour, int value);
    void setMotorValue(int value, bool immediate = true);
    int getMotorValueRaw();
    int setMotorValueRaw(int value);
    void onMotorFinished();

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
    , _sweepValueIndex(0)
    , _displayState(DISPLAY_STATE::CLOCK_MAP)
    , _displayMode(CTRL_DISPLAY_MODE::COS_PHI)
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

    easing_init(
        &_easing,
        EASING_MODE_DEFAULT,
        _easing_calc_InOutQuad,
        0,
        MOTOR_TIMER_PERIOD,
        0);
    easing_set_tick_callback(HAL::GetTick);

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
        setMotorValue(info->motorValue);
        break;

    case CTRL_CMD::SET_CLOCK_MAP:
        return setClockMap(info->hour, info->motorValue);

    case CTRL_CMD::ENABLE_CLOCK_MAP:
        _displayState = DISPLAY_STATE::CLOCK_MAP;
        break;

    case CTRL_CMD::SET_MODE:
        _displayMode = info->displayMode;
        return KVDB_SET(_displayMode);

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
    easing_update(&_easing);
    const int pos = easing_curpos(&_easing);

    setMotorValueRaw(pos);

    /* when easing is finished, stop timer */
    if (easing_isok(&_easing)) {
        _node->stopTimer();
        onMotorFinished();
        HAL_LOG_INFO("Motor value reached: %d", pos);
    }
}

void DP_Ctrl::onGlobalEvent(const Global_Info_t* info)
{
    if (info->event == GLOBAL_EVENT::APP_STARTED) {
        KVDB_GET(_hourMotorMap);
        KVDB_GET(_displayMode);
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

void DP_Ctrl::setMotorValue(int value, bool immediate)
{
    const int currentValue = getMotorValueRaw();
    if (immediate && value == currentValue) {
        return;
    }

    /* Not interrupting current animation */
    if (!easing_isok(&_easing)) {
        return;
    }

#define ABS(x) ((x) < 0 ? -(x) : (x))

    /* Calculate the number of frames to animate */
    _easing.nFrameCount = ABS(value - currentValue) * MOTOR_ANIM_SPEED_FACTOR;

    /* Limit the minimum number of frames */
    if (_easing.nFrameCount < MOTOR_TIMER_PERIOD / 2) {
        _easing.nFrameCount = MOTOR_TIMER_PERIOD / 2;
    }

    easing_start_absolute(&_easing, currentValue, value);
    _node->startTimer(MOTOR_TIMER_PERIOD);
}

int DP_Ctrl::getMotorValueRaw()
{
    int currentValue = 0;
    if (_devMotor->read(&currentValue, sizeof(currentValue)) != sizeof(currentValue)) {
        return 0;
    }

    return currentValue;
}

int DP_Ctrl::setMotorValueRaw(int value)
{
    HAL_LOG_TRACE("value: %d", value);

    if (value < MOTOR_VALUE_MIN || value > MOTOR_VALUE_MAX) {
        HAL_LOG_ERROR("Invalid motor value: %d", value);
        return DataNode::RES_PARAM_ERROR;
    }

    return _devMotor->write(&value, sizeof(value)) == sizeof(value) ? DataNode::RES_OK : DataNode::RES_PARAM_ERROR;
}

void DP_Ctrl::onMotorFinished()
{
    if (_displayState != DISPLAY_STATE::SWEEP_TEST) {
        return;
    }

    static const int16_t testValues[] = {
        0,
        MOTOR_VALUE_MAX,
        MOTOR_VALUE_MIN,
        0,
    };

    if (_sweepValueIndex >= CM_ARRAY_SIZE(testValues)) {
        HAL_LOG_INFO("Sweep test finished");
        return;
    }

    setMotorValue(testValues[_sweepValueIndex], false);
    _sweepValueIndex++;
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
    _sweepValueIndex = 0;
    setMotorValue(0, false);
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
    if (_devBattery->read(&info, sizeof(info)) != sizeof(info)) {
        HAL_LOG_ERROR("Failed to read battery info");
        return;
    }

    HAL_LOG_INFO("voltage: %dmV, level: %d%%", info.voltage, info.level);

    if (_displayMode == CTRL_DISPLAY_MODE::COS_PHI) {
        auto timestamp_0_0_0 = getTimestamp(0);
        auto timestamp_5_0_0 = getTimestamp(5);
        auto timestamp_23_59_59 = getTimestamp(23, 59, 59);

        uint32_t timestamp = 0;
        static const uint32_t demarcationPct = timestamp_5_0_0 * 100 / timestamp_23_59_59;

        if (info.level >= demarcationPct) {
            timestamp = valueMap(info.level, 100, demarcationPct, timestamp_5_0_0, timestamp_23_59_59);
        } else {
            timestamp = valueMap(info.level, demarcationPct, 0, timestamp_0_0_0, timestamp_5_0_0);
        }

        setMotorValue(timestampToMotorValue(timestamp));
    } else if (_displayMode == CTRL_DISPLAY_MODE::LINEAR) {
        uint32_t timestamp = valueMap(info.level, 0, 100, getTimestamp(0), getTimestamp(24));
        setMotorValue(timestampToMotorValue(timestamp));
    }

    _devBattery->ioctl(BATTERY_IOCMD_SLEEP);
}

int DP_Ctrl::timestampToMotorValue(uint32_t timestamp)
{
    if (_displayMode == CTRL_DISPLAY_MODE::COS_PHI) {
        int motorValue = 0;

        if (timestamp >= getTimestamp(5) && timestamp < getTimestamp(7)) {
            motorValue = timestampMap(timestamp, 5, 7, MOTOR_VALUE_H5, MOTOR_VALUE_H7);
        } else if (timestamp >= getTimestamp(7) && timestamp < getTimestamp(9)) {
            motorValue = timestampMap(timestamp, 7, 9, MOTOR_VALUE_H7, MOTOR_VALUE_H9);
        } else if (timestamp >= getTimestamp(9) && timestamp < getTimestamp(12)) {
            motorValue = timestampMap(timestamp, 9, 12, MOTOR_VALUE_H9, MOTOR_VALUE_H12);
        } else if (timestamp >= getTimestamp(12) && timestamp < getTimestamp(21)) {
            motorValue = timestampMap(timestamp, 12, 21, MOTOR_VALUE_H12, MOTOR_VALUE_H21);
        } else if (timestamp >= getTimestamp(21) && timestamp < getTimestamp(24)) {
            motorValue = timestampMap(timestamp, 21, 24, MOTOR_VALUE_H21, MOTOR_VALUE_H0);
        } else if (timestamp >= getTimestamp(0) && timestamp < getTimestamp(1)) {
            motorValue = timestampMap(timestamp, 0, 1, MOTOR_VALUE_H0, MOTOR_VALUE_H1);
        } else if (timestamp >= getTimestamp(1) && timestamp < getTimestamp(5)) {
            motorValue = timestampMap(timestamp, 1, 5, MOTOR_VALUE_H1, MOTOR_VALUE_H5_DOWN);
        }

        return motorValue;
    }

    if (_displayMode == CTRL_DISPLAY_MODE::LINEAR) {
        int motorValue = 0;
        if (timestamp >= getTimestamp(0) && timestamp < getTimestamp(5)) {
            motorValue = timestampMap(timestamp, 0, 5, MOTOR_VALUE_H0, MOTOR_VALUE_H5);
        } else if (timestamp >= getTimestamp(5) && timestamp < getTimestamp(10)) {
            motorValue = timestampMap(timestamp, 5, 10, MOTOR_VALUE_H5, MOTOR_VALUE_H10);
        } else if (timestamp >= getTimestamp(10) && timestamp < getTimestamp(15)) {
            motorValue = timestampMap(timestamp, 10, 15, MOTOR_VALUE_H10, MOTOR_VALUE_H15);
        } else if (timestamp >= getTimestamp(15) && timestamp < getTimestamp(20)) {
            motorValue = timestampMap(timestamp, 15, 20, MOTOR_VALUE_H15, MOTOR_VALUE_H20);
        } else if (timestamp >= getTimestamp(20) && timestamp < getTimestamp(24)) {
            motorValue = timestampMap(timestamp, 20, 24, MOTOR_VALUE_H20, MOTOR_VALUE_H24);
        }
        return motorValue;
    }

    return 0;
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
