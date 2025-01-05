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

using namespace DataProc;

class DP_TimeMonitor {
public:
    DP_TimeMonitor(DataNode* node);

private:
    DataNode* _node;
    const DataNode* _nodeClock;
    int8_t _lastHour;
    int8_t _lastMinute;

private:
    int onEvent(DataNode::EventParam_t* param);
    void onClockEvent(const HAL::Clock_Info_t* info);
};

DP_TimeMonitor::DP_TimeMonitor(DataNode* node)
    : _node(node)
    , _lastHour(-1)
    , _lastMinute(-1)
{
    _nodeClock = _node->subscribe("Clock");
    if (!_nodeClock) {
        return;
    }

    _node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_TimeMonitor*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_PUBLISH);
}

int DP_TimeMonitor::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {
    case DataNode::EVENT_PUBLISH:
        if (param->tran == _nodeClock) {
            onClockEvent((const HAL::Clock_Info_t*)param->data_p);
        }
        break;

    default:
        break;
    }

    return DataNode::RES_OK;
}

void DP_TimeMonitor::onClockEvent(const HAL::Clock_Info_t* info)
{
    if (_lastHour != info->hour) {
        TimeMonitor_Info_t timeMonitorInfo;
        timeMonitorInfo.event = TIME_MONITOR_EVENT::HOUR_CHANGED;
        timeMonitorInfo.clock = info;
        _node->publish(&timeMonitorInfo, sizeof(timeMonitorInfo));
        _lastHour = info->hour;
    }

    if (_lastMinute != info->minute) {
        TimeMonitor_Info_t timeMonitorInfo;
        timeMonitorInfo.event = TIME_MONITOR_EVENT::MINUTE_CHANGED;
        timeMonitorInfo.clock = info;
        _node->publish(&timeMonitorInfo, sizeof(timeMonitorInfo));
        _lastMinute = info->minute;
    }
}

DATA_PROC_DESCRIPTOR_DEF(TimeMonitor)
