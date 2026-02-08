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
#include "MotorCtrl.h"
#include "Frameworks/DataBroker/DataBroker.h"
#include "Service/HAL/HAL_Log.h"
#include "Utils/CommonMacro/CommonMacro.h"

using namespace DataProc;

#define MOTOR_VALUE_MIN -1000
#define MOTOR_VALUE_MAX 1000
#define MOTOR_VALUE_INVALID -32768
#define MOTOR_TIMER_PERIOD 60
#define MOTOR_ANIM_SPEED_FACTOR 0.15f

/* Optimize variables that can be calculated at compile time */
static constexpr int getTimestamp(int hour, int minute = 0, int second = 0)
{
    return hour * 3600 + minute * 60 + second;
}

MotorCtrl::MotorCtrl()
    : _unit(UNIT::NONE)
    , _node(nullptr)
    , _dev(nullptr)
    , _sweepValueIndex(0)
    , _id(0)
    , _displayState(DISPLAY_STATE::CLOCK_MAP)
{
    for (int i = 0; i < CM_ARRAY_SIZE(_valueMap); i++) {
        _valueMap[i] = MOTOR_VALUE_INVALID;
    }

    easing_set_tick_callback(HAL::GetTick);

    easing_init(
        &_easing,
        EASING_MODE_DEFAULT,
        _easing_calc_InOutQuad,
        0,
        MOTOR_TIMER_PERIOD,
        0);
}

int MotorCtrl::setValueMap(uint8_t index, int16_t value)
{
    if (index >= CM_ARRAY_SIZE(_valueMap)) {
        HAL_LOG_ERROR("Invalid index: %d", index);
        return DataNode::RES_PARAM_ERROR;
    }

    if ((value < MOTOR_VALUE_MIN || value > MOTOR_VALUE_MAX) && value != MOTOR_VALUE_INVALID) {
        HAL_LOG_ERROR("Invalid motor value: %d", value);
        return DataNode::RES_PARAM_ERROR;
    }

    HAL_LOG_INFO("INDEX:%d -> M:%d", index, value);

    _valueMap[index] = value;
    listMap();

    return DataNode::RES_OK;
}

void MotorCtrl::setMotorValue(int value, bool immediate)
{
    const int currentValue = getMotorValueRaw();

    /**
     * When immediate is not set, it is necessary to force an update
     * of the animated state machine even if it is with the current value
     */
    if (immediate && value == currentValue) {
        return;
    }

    if (immediate) {
        easing_stop(&_easing, 0);
        setMotorValueRaw(value);
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

void MotorCtrl::update(const HAL::Clock_Info_t* info)
{
    if (_displayState != DISPLAY_STATE::CLOCK_MAP) {
        return;
    }

    int curTimestamp = 0;

    switch (_unit) {
    case UNIT::HOUR:
    case UNIT::HOUR_COS_PHI:
        curTimestamp = getTimestamp(info->hour, info->minute, info->second);
        break;

    case UNIT::MINUTE:
        curTimestamp = info->minute;
        break;

    case UNIT::SECOND:
        curTimestamp = info->second;
        break;

    default:
        return;
    }

    HAL_LOG_TRACE("Current times: %04d-%02d-%02d %02d:%02d:%02d.%03d, timestamp: %d",
        info->year, info->month, info->day, info->hour, info->minute, info->second, info->millisecond, curTimestamp);

    setMotorValue(timestampToMotorValue(curTimestamp));
}

void MotorCtrl::listMap()
{
    HAL_LOG_INFO("ID: %d", _id);
    HAL_LOG_INFO("Unit: %d", _unit);
    for (int i = 0; i < CM_ARRAY_SIZE(_valueMap); i++) {
        if (_valueMap[i] == MOTOR_VALUE_INVALID) {
            continue;
        }
        HAL_LOG_INFO("INDEX:%d -> M:%d", i, _valueMap[i]);
    }
}

void MotorCtrl::sweepTest()
{
    _displayState = DISPLAY_STATE::SWEEP_TEST;
    _sweepValueIndex = 0;
    setMotorValue(0);
}

void MotorCtrl::showLevel(int16_t level)
{
    _displayState = DISPLAY_STATE::BATTERY_USAGE;

    switch (_unit) {
    case UNIT::HOUR_COS_PHI: {
        auto timestamp_0_0_0 = getTimestamp(0);
        auto timestamp_5_0_0 = getTimestamp(5);
        auto timestamp_23_59_59 = getTimestamp(23, 59, 59);

        uint32_t timestamp = 0;
        static const uint32_t demarcationPct = timestamp_5_0_0 * 100 / timestamp_23_59_59;

        if (level >= demarcationPct) {
            timestamp = valueMap(level, 100, demarcationPct, timestamp_5_0_0, timestamp_23_59_59);
        } else {
            timestamp = valueMap(level, demarcationPct, 0, timestamp_0_0_0, timestamp_5_0_0);
        }

        setMotorValue(timestampToMotorValue(timestamp));
    } break;

    case UNIT::HOUR: {
        uint32_t timestamp = valueMap(level, 0, 100, getTimestamp(0), getTimestamp(24));
        setMotorValue(timestampToMotorValue(timestamp));
    } break;

    case UNIT::MINUTE:
    case UNIT::SECOND: {
        /* Map level (0-100) to 0-60 range for minute/second */
        int32_t timeValue = valueMap(level, 0, 100, 0, 60);
        setMotorValue(timestampToMotorValue(timeValue));
    } break;

    default:
        break;
    }
}

bool MotorCtrl::timerHandler()
{
    /* check if animation is still running */
    if (easing_isok(&_easing)) {
        return false;
    }

    easing_update(&_easing);
    const int pos = easing_curpos(&_easing);

    if (setMotorValueRaw(pos) != DataNode::RES_OK) {
        easing_stop(&_easing, 0);
    }

    /* when easing is finished, stop timer */
    if (easing_isok(&_easing)) {
        onMotorFinished();
        HAL_LOG_INFO("Motor[%d] value reached: %d", _id, pos);
        return false;
    }

    return true;
}

int MotorCtrl::getMotorValueRaw()
{
    HAL::Motor_Info_t info;
    if (_dev->read(&info, sizeof(info)) != sizeof(info)) {
        return 0;
    }

    if (_unit == UNIT::HOUR_COS_PHI) {
        return info.value[0] >= 0 ? info.value[0] : -info.value[1];
    }

    return info.value[_id];
}

int MotorCtrl::setMotorValueRaw(int value)
{
    HAL_LOG_TRACE("value: %d", value);

    if (value < MOTOR_VALUE_MIN || value > MOTOR_VALUE_MAX) {
        HAL_LOG_ERROR("Invalid motor value: %d", value);
        return DataNode::RES_PARAM_ERROR;
    }

    HAL::Motor_Info_t info = { -1, -1 };

    if (_unit == UNIT::HOUR_COS_PHI) {
        info.value[0] = value >= 0 ? value : 0;
        info.value[1] = value < 0 ? -value : 0;
    } else {
        info.value[_id] = value;
    }

    return _dev->write(&info, sizeof(info)) == sizeof(info) ? DataNode::RES_OK : DataNode::RES_PARAM_ERROR;
}

void MotorCtrl::onMotorFinished()
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

    setMotorValue(testValues[_sweepValueIndex]);
    _sweepValueIndex++;
}

int MotorCtrl::timestampToMotorValue(int timestamp)
{
    switch (_unit) {
    case UNIT::HOUR_COS_PHI: {
        int motorValue = 0;

        if (timestamp >= getTimestamp(5) && timestamp < getTimestamp(7)) {
            motorValue = timestampMap(timestamp, 5, 7);
        } else if (timestamp >= getTimestamp(7) && timestamp < getTimestamp(9)) {
            motorValue = timestampMap(timestamp, 7, 9);
        } else if (timestamp >= getTimestamp(9) && timestamp < getTimestamp(12)) {
            motorValue = timestampMap(timestamp, 9, 12);
        } else if (timestamp >= getTimestamp(12) && timestamp < getTimestamp(21)) {
            motorValue = timestampMap(timestamp, 12, 21);
        } else if (timestamp >= getTimestamp(21) && timestamp < getTimestamp(24)) {
            motorValue = timestampMap(timestamp, 21, 24, _valueMap[21], _valueMap[0]);
        } else if (timestamp >= getTimestamp(0) && timestamp < getTimestamp(1)) {
            motorValue = timestampMap(timestamp, 0, 1);
        } else if (timestamp >= getTimestamp(1) && timestamp < getTimestamp(5)) {
            motorValue = timestampMap(timestamp, 1, 5, _valueMap[1], _valueMap[24]);
        }

        return motorValue;
    }

    case UNIT::HOUR: {
        const int currentHour = timestamp / 3600;
        if (currentHour >= 24) {
            return timestampMap(timestamp, 24, 24);
        }

        int prevHour = -1;
        for (int i = currentHour; i >= 0; i--) {
            if (_valueMap[i] != MOTOR_VALUE_INVALID) {
                prevHour = i;
                break;
            }
        }

        int nextHour = -1;
        for (int i = currentHour + 1; i < CM_ARRAY_SIZE(_valueMap); i++) {
            if (_valueMap[i] != MOTOR_VALUE_INVALID) {
                nextHour = i;
                break;
            }
        }

        if (prevHour < 0 || nextHour < 0) {
            HAL_LOG_ERROR("currentHour: %d not found in hourMotorMap", currentHour);
            listMap();
            return 0;
        }

        HAL_LOG_TRACE("currentHour: %d, prevHour: %d, nextHour: %d", currentHour, prevHour, nextHour);
        return timestampMap(timestamp, prevHour, nextHour);
    } break;

    case UNIT::MINUTE:
    case UNIT::SECOND: {
        /*
         * For MINUTE/SECOND unit, timestamp here is 0-60 value
         * _valueMap[0]~_valueMap[6] maps to 0, 10, 20, 30, 40, 50, 60
         */
        const int mapIndex = timestamp / 10; /* 0-6 */
        if (mapIndex >= 6) {
            return _valueMap[6];
        }

        if (mapIndex < 0) {
            return _valueMap[0];
        }

        /* Check if current and next index are valid */
        if (_valueMap[mapIndex] == MOTOR_VALUE_INVALID || _valueMap[mapIndex + 1] == MOTOR_VALUE_INVALID) {
            HAL_LOG_ERROR("Invalid valueMap at index: %d or %d", mapIndex, mapIndex + 1);
            return 0;
        }

        /* Interpolate between mapIndex and mapIndex+1 */
        int32_t min_in = mapIndex * 10;
        int32_t max_in = (mapIndex + 1) * 10;
        return valueMap(timestamp, min_in, max_in, _valueMap[mapIndex], _valueMap[mapIndex + 1]);
    } break;

    default:
        break;
    }

    return 0;
}

int32_t MotorCtrl::timestampMap(int32_t x, int32_t hour_start, int32_t hour_end, int32_t min_out, int32_t max_out)
{
    int32_t min_in = getTimestamp(hour_start);
    int32_t max_in = getTimestamp(hour_end);
    return valueMap(x, min_in, max_in, min_out, max_out);
}

int32_t MotorCtrl::timestampMap(int32_t x, int32_t hour_start, int32_t hour_end)
{
    if (hour_start >= CM_ARRAY_SIZE(_valueMap) || hour_end >= CM_ARRAY_SIZE(_valueMap) || hour_start < 0 || hour_end < 0) {
        HAL_LOG_ERROR("Invalid hour: %d, %d", hour_start, hour_end);
        return 0;
    }

    int32_t min_in = getTimestamp(hour_start);
    int32_t max_in = getTimestamp(hour_end);
    return valueMap(x, min_in, max_in, _valueMap[hour_start], _valueMap[hour_end]);
}

int32_t MotorCtrl::valueMap(int32_t x, int32_t min_in, int32_t max_in, int32_t min_out, int32_t max_out)
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
