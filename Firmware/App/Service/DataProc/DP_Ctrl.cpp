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
#include "Helper/MotorCtrl.h"
#include "Service/HAL/HAL.h"
#include "Service/HAL/HAL_Log.h"
#include "Utils/CommonMacro/CommonMacro.h"

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
    const DataNode* _nodeButton;
    KVDB_Helper _kvdb;
    DeviceObject* _devMotor;
    DeviceObject* _devBattery;

    MotorCtrl _mctrl[2];

private:
    int onEvent(DataNode::EventParam_t* param);
    int onNotify(const Ctrl_Info_t* info);
    void onTimer();
    void onClockEvent(const HAL::Clock_Info_t* info);
    void onGlobalEvent(const Global_Info_t* info);
    void onButtonEvent(const Button_Info_t* info);

    void showBatteryUsage();
    int setUnit(uint8_t id, MotorCtrl::UNIT unit);
    int setClockMap(uint8_t id, int hour, int value);
};

DP_Ctrl::DP_Ctrl(DataNode* node)
    : _node(node)
    , _kvdb(node)
{
    _devMotor = HAL::Manager()->getDevice("Motor");
    if (!_devMotor) {
        return;
    }

    for (int i = 0; i < CM_ARRAY_SIZE(_mctrl); i++) {
        _mctrl[i].setID(i);
        _mctrl[i].setNode(node);
        _mctrl[i].setDevice(_devMotor);
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
    if (info->motorID >= CM_ARRAY_SIZE(_mctrl)) {
        HAL_LOG_ERROR("Invalid motor ID: %d", info->motorID);
        return DataNode::RES_PARAM_ERROR;
    }

    switch (info->cmd) {
    case CTRL_CMD::SWEEP_TEST:
        _mctrl[info->motorID].sweepTest();
        break;

    case CTRL_CMD::SET_MOTOR_VALUE:
        _mctrl[info->motorID].setDisplayState(MotorCtrl::DISPLAY_STATE::MOTOR_SET);
        _mctrl[info->motorID].setMotorValue(info->motorValue, info->immediate);
        break;

    case CTRL_CMD::SET_CLOCK_MAP:
        return setClockMap(info->motorID, info->hour, info->motorValue);

    case CTRL_CMD::ENABLE_CLOCK_MAP:
        _mctrl[info->motorID].setDisplayState(MotorCtrl::DISPLAY_STATE::CLOCK_MAP);
        break;

    case CTRL_CMD::LIST_CLOCK_MAP:
        _mctrl[info->motorID].listMap();
        break;

    case CTRL_CMD::SET_UNIT:
        return setUnit(info->motorID, info->unit);

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
    int activeMotorCount = 0;
    for (int i = 0; i < CM_ARRAY_SIZE(_mctrl); i++) {
        activeMotorCount += _mctrl[i].timerHandler();
    }

    if (activeMotorCount == 0) {
        _node->stopTimer();
    }
}

void DP_Ctrl::onGlobalEvent(const Global_Info_t* info)
{
    if (info->event == GLOBAL_EVENT::APP_STARTED) {
        KVDB_GET(_mctrl[0]._valueMap);
        KVDB_GET(_mctrl[0]._unit);
        KVDB_GET(_mctrl[1]._valueMap);
        KVDB_GET(_mctrl[1]._unit);
        _mctrl[0].listMap();
        _mctrl[1].listMap();
    }
}

int DP_Ctrl::setUnit(uint8_t id, MotorCtrl::UNIT unit)
{
    _mctrl[id].setUnit(unit);
    HAL_LOG_INFO("Set motor %d unit to %d", id, unit);

    if (id == 0) {
        return KVDB_SET(_mctrl[0]._unit);
    } else if (id == 1) {
        return KVDB_SET(_mctrl[1]._unit);
    }

    return DataNode::RES_PARAM_ERROR;
}

int DP_Ctrl::setClockMap(uint8_t id, int hour, int value)
{
    auto ret = _mctrl[id].setValueMap(hour, value);
    if (ret != DataNode::RES_OK) {
        return ret;
    }

    /* In order to match the KVDB key string, it is necessary to specify the ID. */
    if (id == 0) {
        return KVDB_SET(_mctrl[0]._valueMap);
    } else if (id == 1) {
        return KVDB_SET(_mctrl[1]._valueMap);
    }

    return DataNode::RES_PARAM_ERROR;
}

void DP_Ctrl::onClockEvent(const HAL::Clock_Info_t* info)
{
    _mctrl[0].update(info);
    _mctrl[1].update(info);
}

void DP_Ctrl::onButtonEvent(const Button_Info_t* info)
{
    switch (info->event) {
    case BUTTON_EVENT::PRESSED:
        showBatteryUsage();
        break;

    case BUTTON_EVENT::RELEASED:
        _mctrl[0].setDisplayState(MotorCtrl::DISPLAY_STATE::CLOCK_MAP);
        _mctrl[1].setDisplayState(MotorCtrl::DISPLAY_STATE::CLOCK_MAP);
        break;

    default:
        break;
    }
}

void DP_Ctrl::showBatteryUsage()
{
    if (!_devBattery) {
        HAL_LOG_WARN("No battery device found");
        return;
    }

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

    _mctrl[0].showLevel(info.level);

    _devBattery->ioctl(BATTERY_IOCMD_SLEEP);
}

DATA_PROC_DESCRIPTOR_DEF(Ctrl)
