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
#include "Service/HAL/HAL_Log.h"
#include "Utils/CommonMacro/CommonMacro.h"
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
    int8_t _lastMinute;

    Audio_Squence_t _seq[4];

private:
    int onEvent(DataNode::EventParam_t* param);
    void onClockEvent(const HAL::Clock_Info_t* info);
    void onHourChanged(int8_t hour);
    void onMinuteChanged(int8_t hour, int8_t minute);
};

DP_TimeMonitor::DP_TimeMonitor(DataNode* node)
    : _node(node)
    , _audio(node)
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
    //    static int h = 0;
    //    h = (h + 1) % 24;
    //    onHourChanged(h);

    if (_lastHour != info->hour) {
        onHourChanged(info->hour);
        _lastHour = info->hour;
    }

    if (_lastMinute != info->minute) {
        onMinuteChanged(info->hour, info->minute);
        _lastMinute = info->minute;
    }
}

void DP_TimeMonitor::onHourChanged(int8_t hour)
{
    if (hour > 0 && hour < 10) {
        return;
    }

    static const uint16_t hourMap[] = {
        ToneMap::L1,
        ToneMap::L3,
        ToneMap::L5,
        ToneMap::M1,
        ToneMap::M3,
        ToneMap::M5,
        ToneMap::H1,
        ToneMap::H3,
        ToneMap::H5,
    };

    // static const uint16_t hourMap[] = {
    //     ToneMap::L3,
    //     ToneMap::L5,
    //     ToneMap::L7,
    //     ToneMap::M2,
    //     ToneMap::M4,
    //     ToneMap::M6,
    //     ToneMap::H1,
    //     ToneMap::H3,
    //     ToneMap::H5,
    // };

    const uint32_t hourIndexMax = sizeof(hourMap) / sizeof(hourMap[0]) - 1;

    _seq[0].frequency = hourMap[hour / hourIndexMax];
    _seq[0].duration = ToneMap::BEAT_1_4;
    _seq[1].frequency = hourMap[hour % hourIndexMax + 1];
    _seq[1].duration = ToneMap::BEAT_1_4;
    _seq[2].frequency = hourMap[hour / hourIndexMax + 1];
    _seq[2].duration = ToneMap::BEAT_1_4;
    _seq[3].frequency = hourMap[hour % hourIndexMax];
    _seq[3].duration = ToneMap::BEAT_1_4;
    _audio.play(AUDIO_HELPER_SEQ_DEF(_seq));
}

void DP_TimeMonitor::onMinuteChanged(int8_t hour, int8_t minute)
{
#define TONE_DUTY_CYCLE 0.8f
#define TONE_BEAT_MAKE(beat) ((uint32_t)((beat) * TONE_DUTY_CYCLE)), (beat)

    const Audio_Squence_t* seq_alarm = nullptr;
    int seq_alarm_len = 0;
    int seq_bpm = 0;

    if (hour == 12 && minute == 30) {
        static const Audio_Squence_t seq_mtag[] = {
            { ToneMap::M1, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::M1, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::M5, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::M5, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::L6h, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::L6h, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
            { ToneMap::M2h, TONE_BEAT_MAKE(ToneMap::BEAT_1_4 + ToneMap::BEAT_1_8) },
        };

        seq_alarm = seq_mtag;
        seq_alarm_len = CM_ARRAY_SIZE(seq_mtag);
    } else if (hour == 18 && minute == 30) {
        static const Audio_Squence_t seq_mc_bgm[] = {
            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
            { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H1, TONE_BEAT_MAKE(ToneMap::BEAT_1_2) },
            { ToneMap::M6h, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
            { 0, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },

            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
            { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H1, TONE_BEAT_MAKE(ToneMap::BEAT_1_2) },
            { ToneMap::H2h, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
            { 0, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },

//            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
//            { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
//            { ToneMap::H1, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
//            { ToneMap::H2h, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
//            { ToneMap::M6h, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
//            { 0, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },

//            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
//            { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
//            { ToneMap::H1, TONE_BEAT_MAKE(ToneMap::BEAT_1_2) },
//            { ToneMap::H2h, TONE_BEAT_MAKE(ToneMap::BEAT_1_2 + ToneMap::BEAT_1_4) },
        };

        seq_alarm = seq_mc_bgm;
        seq_alarm_len = CM_ARRAY_SIZE(seq_mc_bgm);
        seq_bpm = 50;
    } else if (hour == 21) {
        static const Audio_Squence_t seq_gta4_phone[] = {
            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
            { ToneMap::H6h, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
            { ToneMap::H5h, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
            { ToneMap::H5, TONE_BEAT_MAKE(ToneMap::BEAT_1_2) },
            { 0, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },

            { ToneMap::H2, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H2, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H2, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
            { ToneMap::H1h, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H1, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H1h, TONE_BEAT_MAKE(ToneMap::BEAT_1_8) },
            { ToneMap::H3, TONE_BEAT_MAKE(ToneMap::BEAT_1_4) },
            { ToneMap::H4, TONE_BEAT_MAKE(ToneMap::BEAT_1_4 + ToneMap::BEAT_1_8) },
        };
        seq_alarm = seq_gta4_phone;
        seq_alarm_len = CM_ARRAY_SIZE(seq_gta4_phone);
        seq_bpm = 50;
    }

    if (!seq_alarm) {
        return;
    }

    HAL_LOG_INFO("Play alarm sound: %p, len: %d", seq_alarm, seq_alarm_len);
    _audio.play(seq_alarm, seq_alarm_len, seq_bpm);
}

DATA_PROC_DESCRIPTOR_DEF(TimeMonitor)
