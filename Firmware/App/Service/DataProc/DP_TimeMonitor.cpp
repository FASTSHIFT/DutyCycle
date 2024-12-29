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
#include "Utils/ToneMap/ToneMap.h"

using namespace DataProc;

class DP_TimeMonitor {
public:
    DP_TimeMonitor(DataNode* node);

private:
    DataNode* _node;
    const DataNode* _nodeClock;
    Audio_Helper _audio;
    int8_t _lastHour;

    Audio_Squence_t _seq[4];

private:
    int onEvent(DataNode::EventParam_t* param);
    void onClockEvent(const HAL::Clock_Info_t* info);
    void onHourChanged(int hour);
};

DP_TimeMonitor::DP_TimeMonitor(DataNode* node)
    : _node(node)
    , _audio(node)
    , _lastHour(-1)
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

//    static int h = 0;
//    h = (h + 1) % 24;
//    onHourChanged(h);

    if (_lastHour == info->hour) {
        return;
    }

    onHourChanged(info->hour);

    _lastHour = info->hour;
}

void DP_TimeMonitor::onHourChanged(int hour)
{
    if (hour > 23) {
        return;
    }

    static const uint16_t hourMap[] = {
        M1,
        M1h,
        M2,
        M2h,
        M3,
        M4,
        M4h,
        M5,
        M5h,
        M6,
        M6h,
        M7,
    };

    _seq[0].frequency = hourMap[hour / 12];
    _seq[0].duration = 100;
    _seq[0].duration = 50;
    _seq[1].frequency = hourMap[hour % 12];
    _seq[1].duration = 100;
    _seq[1].duration = 50;
    _seq[2].frequency = hourMap[hour / 12];
    _seq[2].duration = 100;
    _seq[2].duration = 50;
    _seq[3].frequency = hourMap[hour % 12];
    _seq[3].duration = 100;
    _seq[3].duration = 50;
    _audio.play(AUDIO_HELPER_SEQ_DEF(_seq));
}

DATA_PROC_DESCRIPTOR_DEF(TimeMonitor)
