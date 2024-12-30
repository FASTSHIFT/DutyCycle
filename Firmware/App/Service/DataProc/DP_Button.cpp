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
#include "Utils/ButtonEvent/ButtonEvent.h"
#include "Utils/ToneMap/ToneMap.h"

using namespace DataProc;

class DP_Button {
public:
    DP_Button(DataNode* node);

private:
    DataNode* _node;
    const DataNode* _nodeGlobal;
    Audio_Helper _audio;
    DeviceObject* _dev;
    ButtonEvent _btnOK;
    bool _waitRelease;

private:
    int onEvent(DataNode::EventParam_t* param);
    int onTimer();
    int onGlobalEvent(const Global_Info_t* info);
    static void onBtnEvent(ButtonEvent* btn, ButtonEvent::Event_t event);
    void onBtnOkEvent(const DataProc::Button_Info_t* info);
};

DP_Button::DP_Button(DataNode* node)
    : _node(node)
    , _audio(node)
    , _dev(nullptr)
    , _waitRelease(true)
{
    _dev = HAL::Manager()->getDevice("Button");

    if (!_dev) {
        return;
    }

    _nodeGlobal = _node->subscribe("Global");

    /* init button object */
    ButtonEvent::setTickGetterCallback(HAL::GetTick);
    _btnOK.setEventCallback(onBtnEvent);
    _btnOK.setUserData(this);

    node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) {
            auto ctx = (DP_Button*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_TIMER | DataNode::EVENT_PULL | DataNode::EVENT_PUBLISH);
}

int DP_Button::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {
    case DataNode::EVENT_PUBLISH: {
        if (param->tran == _nodeGlobal) {
            return onGlobalEvent((const Global_Info_t*)param->data_p);
        }
    } break;

    case DataNode::EVENT_TIMER:
        return onTimer();

    case DataNode::EVENT_PULL: {
        if (param->size != sizeof(HAL::Button_Info_t)) {
            return DataNode::RES_SIZE_MISMATCH;
        }
        HAL::Button_Info_t btnInfo;
        if (_dev->read(&btnInfo, sizeof(btnInfo)) != sizeof(btnInfo)) {
            return DataNode::RES_NO_DATA;
        }
        memcpy(param->data_p, &btnInfo, sizeof(btnInfo));
    } break;
    default:
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }

    return DataNode::RES_OK;
}

int DP_Button::onGlobalEvent(const Global_Info_t* info)
{
    if (info->event != GLOBAL_EVENT::APP_RUN_LOOP_BEGIN) {
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }

    if (_node->isTimerRunning()) {
        return DataNode::RES_OK;
    }

    HAL::Button_Info_t btnInfo;
    if (_dev->read(&btnInfo, sizeof(btnInfo)) != sizeof(btnInfo)) {
        return DataNode::RES_NO_DATA;
    }

    if (HAL::GetTickElaps(btnInfo.lastActiveTick) < 100) {
        HAL_LOG_INFO("Button pressed, start monitoring...");
        _node->startTimer(20);
    }

    return DataNode::RES_OK;
}

int DP_Button::onTimer()
{
    HAL::Button_Info_t info;
    if (_dev->read(&info, sizeof(info)) != sizeof(info)) {
        return DataNode::RES_NO_DATA;
    }

    if (_waitRelease && info.key.ok) {
        /* wait for button release */
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }
    _waitRelease = false;

    _btnOK.monitor(info.key.ok);

    if (info.value == 0 && HAL::GetTickElaps(info.lastActiveTick) > 500) {
        HAL_LOG_INFO("Timeout, stop monitoring...");
        _node->stopTimer();
    }
    return DataNode::RES_OK;
}

void DP_Button::onBtnEvent(ButtonEvent* btn, ButtonEvent::Event_t event)
{
    auto ctx = (DP_Button*)btn->getUserData();
    DataProc::Button_Info_t info;
    info.event = (DataProc::BUTTON_EVENT)event;

    if (btn == &ctx->_btnOK) {
        info.id = BUTTON_ID::OK;
        ctx->onBtnOkEvent(&info);
    } else {
        return;
    }

    ctx->_node->publish(&info, sizeof(info));
}

void DP_Button::onBtnOkEvent(const DataProc::Button_Info_t* info)
{
    switch (info->event) {
    case DataProc::BUTTON_EVENT::PRESSED: {
        static const Audio_Squence_t seq[] = {
            { L7, 20 }
        };

        _audio.play(AUDIO_HELPER_SEQ_DEF(seq));

    } break;

    case DataProc::BUTTON_EVENT::RELEASED: {
        static const Audio_Squence_t seq[] = {
            { M4, 20 }
        };

        _audio.play(AUDIO_HELPER_SEQ_DEF(seq));
    } break;

    default:
        return;
    }
}

DATA_PROC_DESCRIPTOR_DEF(Button)
