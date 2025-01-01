/*
 * MIT License
 * Copyright (c) 2021 - 2023 _VIFEXTech
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
#include "Config/Config.h"
#include "DataProc.h"
#include "Service/HAL/HAL.h"
#include "Service/HAL/HAL_Log.h"
#include <stdio.h>
#include <stdlib.h>

using namespace DataProc;

class DP_Clock {
public:
    DP_Clock(DataNode* node);

private:
    DataNode* _node;
    DeviceObject* _dev;
    int _timeZone;

private:
    int onEvent(DataNode::EventParam_t* param);
    int onNotify(const Clock_Info_t* info);
    void setCompileTimeToClock();
};

DP_Clock::DP_Clock(DataNode* node)
    : _node(node)
    , _dev(nullptr)
{
    _dev = HAL::Manager()->getDevice("Clock");
    if (!_dev) {
        return;
    }

    setCompileTimeToClock();

    _node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) {
            auto ctx = (DP_Clock*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_PULL | DataNode::EVENT_NOTIFY | DataNode::EVENT_TIMER);

    node->startTimer(2000);
}

int DP_Clock::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {
    case DataNode::EVENT_PULL: {
        /* Clock info pull request */
        int ret = _dev->read(param->data_p, param->size);
        return ret == sizeof(HAL::Clock_Info_t) ? DataNode::RES_OK : DataNode::RES_NO_DATA;
    }

    case DataNode::EVENT_NOTIFY: {
        if (param->size != sizeof(Clock_Info_t)) {
            return DataNode::RES_SIZE_MISMATCH;
        }
        return onNotify((const Clock_Info_t*)param->data_p);
    }

    case DataNode::EVENT_TIMER: {
        HAL::Clock_Info_t clock;
        int ret = _dev->read(&clock, sizeof(clock));
        if (ret != sizeof(clock)) {
            return DataNode::RES_NO_DATA;
        }
        return _node->publish(&clock, sizeof(clock));
    }

    default:
        break;
    }

    return DataNode::RES_UNKNOWN;
}

int DP_Clock::onNotify(const Clock_Info_t* info)
{
    switch (info->cmd) {
    case CLOCK_CMD::SET_TIME: {
        int retval = _dev->ioctl(CLOCK_IOCMD_CALIBRATE, (void*)&(info->base), sizeof(info->base));
        return retval == DeviceObject::RES_OK ? DataNode::RES_OK : DataNode::RES_NO_DATA;
    }

    case CLOCK_CMD::SET_ALARM: {
        int retval = _dev->ioctl(CLOCK_IOCMD_SET_ALARM, (void*)&(info->base), sizeof(info->base));
        return retval == DeviceObject::RES_OK ? DataNode::RES_OK : DataNode::RES_NO_DATA;
    }

    case CLOCK_CMD::GET_ALARM:
        break;

    case CLOCK_CMD::DISABLE_ALARM:
        break;

    default:
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }

    return DataNode::RES_OK;
}

void DP_Clock::setCompileTimeToClock()
{
    HAL_LOG_INFO("Build: %s %s", __DATE__, __TIME__);

    int day, year;
    char month[4];

    sscanf(__DATE__, "%3s %d %d", month, &day, &year); // "Sep 29 2023"

    int month_int = 0;
    static const char* month_names[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    for (int i = 0; i < sizeof(month_names) / sizeof(month_names[0]); i++) {
        if (strcmp(month, month_names[i]) == 0) {
            month_int = i + 1;
            break;
        }
    }

    int hour, minute, second;
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second); // "14:55:30"

    HAL::Clock_Info_t info;
    memset(&info, 0, sizeof(info));
    info.year = year;
    info.month = month_int;
    info.day = day;
    info.hour = hour;
    info.minute = minute;
    info.second = second;
    _dev->ioctl(CLOCK_IOCMD_CALIBRATE, &info, sizeof(info));
}

DATA_PROC_DESCRIPTOR_DEF(Clock)
