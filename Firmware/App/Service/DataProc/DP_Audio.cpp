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
#include "Utils/ToneMap/ToneMap.h"

using namespace DataProc;

class DP_Audio {
public:
    DP_Audio(DataNode* node);

private:
    DataNode* _node;
    DeviceObject* _devBuzz;

    Audio_Info_t _info;
    uint32_t _curIndex;

private:
    int onEvent(DataNode::EventParam_t* param);
    void onTimer();
    void writeBuzzer(uint32_t freq, uint32_t duration);
    int start(const Audio_Info_t* info);
    void stop();
};

DP_Audio::DP_Audio(DataNode* node)
    : _node(node)
    , _curIndex(0)
{
    _devBuzz = HAL::Manager()->getDevice("Buzzer");
    if (!_devBuzz) {
        return;
    }

    _node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_Audio*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_PULL | DataNode::EVENT_NOTIFY | DataNode::EVENT_TIMER);

    static const Audio_Squence_t squence[] = {
        { ToneMap::M1, 80 },
        { ToneMap::M6, 80 },
        { ToneMap::M3, 80 },
    };

    Audio_Info_t startUpInfo;
    startUpInfo.squence = squence;
    startUpInfo.length = sizeof(squence) / sizeof(Audio_Squence_t);
    start(&startUpInfo);
}

int DP_Audio::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {
    case DataNode::EVENT_PULL:
        if (param->size != sizeof(_info)) {
            return DataNode::RES_SIZE_MISMATCH;
        }
        memcpy(param->data_p, &_info, sizeof(_info));
        break;

    case DataNode::EVENT_NOTIFY: {
        if (param->size != sizeof(_info)) {
            return DataNode::RES_SIZE_MISMATCH;
        }
        start((const Audio_Info_t*)param->data_p);
    } break;

    case DataNode::EVENT_TIMER:
        onTimer();
        break;

    default:
        break;
    }

    return DataNode::RES_OK;
}

void DP_Audio::writeBuzzer(uint32_t freq, uint32_t duration)
{
    HAL::Buzzer_Info_t info;
    info.freq = freq;
    info.duration = duration;
    _devBuzz->write(&info, sizeof(info));
}

int DP_Audio::start(const Audio_Info_t* info)
{
    if (info->bpm == 0) {
        HAL_LOG_ERROR("bpm need > 0");
        return DataNode::RES_PARAM_ERROR;
    }

    if (_info.squence && !_info.interruptible) {
        HAL_LOG_WARN("Audio is playing, can't interrupt");
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }

    _curIndex = 0;
    _info = *info;

    if (_info.squence == nullptr || _info.length == 0) {
        stop();
        return DataNode::RES_OK;
    }

    HAL_LOG_INFO("Audio start: %p, length: %d, bpm: %d, interruptible: %d", _info.squence, _info.length, _info.bpm, _info.interruptible);

    _node->startTimer(0);
    return DataNode::RES_OK;
}

void DP_Audio::stop()
{
    HAL_LOG_INFO("Audio stop: %p", _info.squence);
    _info.squence = nullptr;
    _info.length = 0;
    _node->stopTimer();
    writeBuzzer(0, 0);
}

void DP_Audio::onTimer()
{
    if (_curIndex >= _info.length) {
        stop();
    }

    auto cur = &_info.squence[_curIndex++];
    writeBuzzer(cur->frequency, cur->duration * AUDIO_BPM_DEFAULT / _info.bpm);

    const uint32_t period = cur->time > cur->duration ? cur->time : cur->duration;
    _node->setTimerPeriod(period * AUDIO_BPM_DEFAULT / _info.bpm);
}

DATA_PROC_DESCRIPTOR_DEF(Audio)
