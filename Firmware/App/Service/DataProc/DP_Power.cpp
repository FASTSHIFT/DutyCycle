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

class DP_Power {
public:
    DP_Power(DataNode* node);

private:
    DataNode* _node;
    const DataNode* _nodeGlobal;
    const DataNode* _nodeButton;
    DeviceObject* _devPower;
    DeviceObject* _devBattery;
    DeviceObject* _devTick;

    Power_Info_t _info;
    uint32_t _lastTick;
    uint32_t _wakeUpTick;
    int _lockCount;

private:
    int onEvent(DataNode::EventParam_t* param);
    int onGlobalEvent(const Global_Info_t* info);
    int onButtonEvent(const Button_Info_t* info);
    void onPowerNotify(const Power_Info_t* info);
    void checkShutdown();
    void onShutdown();
};

DP_Power::DP_Power(DataNode* node)
    : _node(node)
{
    _devPower = HAL::Manager()->getDevice("Power");
    if (!_devPower) {
        HAL_LOG_ERROR("Failed to get Power device");
        return;
    }

    _devBattery = HAL::Manager()->getDevice("Battery");
    if (!_devBattery) {
        HAL_LOG_ERROR("Failed to get Battery device");
        return;
    }

    _devTick = HAL::Manager()->getDevice("Tick");
    if (!_devTick) {
        HAL_LOG_ERROR("Failed to get Tick device");
        return;
    }

    _nodeGlobal = _node->subscribe("Global");
    _nodeButton = _node->subscribe("Button");

    _node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_Power*)n->getUserData();
            return ctx->onEvent(param);
        });

//    _node->startTimer(1000);

    _lastTick = HAL::GetTick();
    _wakeUpTick = HAL::GetTick();

    _info.autoShutdownTime = 0;
}

int DP_Power::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {
    case DataNode::EVENT_TIMER:
        checkShutdown();
        break;

    case DataNode::EVENT_PULL: {
        if (param->size != sizeof(Power_Info_t)) {
            return DataNode::RES_SIZE_MISMATCH;
        }

        /* read power info */
        HAL::Battery_Info_t info;
        if (_devBattery->read(&info, sizeof(info)) != sizeof(info)) {
            return DataNode::RES_NO_DATA;
        }

        /* basic info */
        _info.cmd = POWER_CMD::UPDATE_INFO;
        _info.voltage = info.voltage;
        _info.level = info.level;
        _info.isReady = info.isReady;
        _info.isCharging = info.isCharging;

        memcpy(param->data_p, &_info, sizeof(Power_Info_t));
    } break;

    case DataNode::EVENT_NOTIFY:
        if (param->size != sizeof(Power_Info_t)) {
            return DataNode::RES_SIZE_MISMATCH;
        }
        onPowerNotify((Power_Info_t*)param->data_p);
        break;

    case DataNode::EVENT_PUBLISH:
        if (param->tran == _nodeGlobal) {
            return onGlobalEvent((Global_Info_t*)param->data_p);
        } else if (param->tran == _nodeButton) {
            return onButtonEvent((Button_Info_t*)param->data_p);
        }
        break;

    default:
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }

    return DataNode::RES_OK;
}

int DP_Power::onGlobalEvent(const Global_Info_t* info)
{
    switch (info->event) {
    case GLOBAL_EVENT::APP_RUN_LOOP_BEGIN:
        _devTick->ioctl(TICK_IOCMD_STOP);
        break;

    case GLOBAL_EVENT::APP_RUN_LOOP_END: {
        _devTick->ioctl(TICK_IOCMD_START, info->param, sizeof(uint32_t));
        _devPower->ioctl(POWER_IOCMD_WFI);
    }
    break;

    default:
        break;
    }

    return DataNode::RES_OK;
}

int DP_Power::onButtonEvent(const Button_Info_t* info)
{
    switch (info->event) {
    case BUTTON_EVENT::LONG_PRESSED:
        // onShutdown();
        break;

    default:
        break;
    }

    return DataNode::RES_OK;
}

void DP_Power::onPowerNotify(const Power_Info_t* info)
{
    switch (info->cmd) {
    case POWER_CMD::SHUTDOWN:
        onShutdown();
        break;

    case POWER_CMD::REBOOT:
        _devPower->ioctl(POWER_IOCMD_REBOOT);
        break;

    case POWER_CMD::LOCK_WAKEUP:
        _lockCount++;
        HAL_LOG_INFO("Lock wakeup, count = %d", _lockCount);
        break;

    case POWER_CMD::UNLOCK_WAKEUP:
        _lockCount--;
        HAL_LOG_INFO("Unlock wakeup, count = %d", _lockCount);

        if (_lockCount < 0) {
            HAL_LOG_WARN("Error unlock wakeup");
            _lockCount = 0;
        }

        /* update wake up tick */
        _wakeUpTick = HAL::GetTick();
        break;

    case POWER_CMD::KICK_WAKUP:
        _wakeUpTick = HAL::GetTick();
        break;

    case POWER_CMD::SET_AUTO_SHUTDOWN_TIME: {
        _info.autoShutdownTime = info->autoShutdownTime;
    } break;

    default:
        break;
    }
}

void DP_Power::checkShutdown()
{
    /* check auto shutdown */
    if (_info.autoShutdownTime > 0 && _lockCount == 0) {
        if (HAL::GetTickElaps(_wakeUpTick) > _info.autoShutdownTime * 1000U) {
            HAL_LOG_WARN("Auto shutdown after %dsec", _info.autoShutdownTime);
            onShutdown();
        }
    }
}

void DP_Power::onShutdown()
{
    _info.cmd = POWER_CMD::SHUTDOWN;
    if (_node->publish(&_info, sizeof(_info)) == DataNode::RES_STOP_PROCESS) {
        HAL_LOG_WARN("Stop shutdown process");
        return;
    }

    _devBattery->ioctl(BATTERY_IOCMD_SLEEP);
    _devPower->ioctl(POWER_IOCMD_POWER_OFF);
    _node->stopTimer();
}

DATA_PROC_DESCRIPTOR_DEF(Power)
