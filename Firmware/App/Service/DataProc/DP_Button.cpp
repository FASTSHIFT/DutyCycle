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
#include "Utils/ButtonEvent/ButtonEvent.h"

using namespace DataProc;

class DP_Button {
public:
    DP_Button(DataNode* node);

private:
    DataNode* _node;
    DeviceObject* _dev;
    ButtonEvent _btnOK;
    bool _waitRelease;

private:
    int onEvent(DataNode::EventParam_t* param);
    static void onBtnEvent(ButtonEvent* btn, ButtonEvent::Event_t event);
};

DP_Button::DP_Button(DataNode* node)
    : _node(node)
    , _dev(nullptr)
    , _waitRelease(true)
{
    _dev = HAL::Manager()->getDevice("Button");

    if (!_dev) {
        return;
    }

    /* init button object */
    ButtonEvent::setTickGetterCallback(HAL::GetTick);
    _btnOK.setEventCallback(onBtnEvent);
    _btnOK.setUserData(this);

    node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) {
            auto ctx = (DP_Button*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_TIMER | DataNode::EVENT_PULL);
    node->startTimer(20);
}

int DP_Button::onEvent(DataNode::EventParam_t* param)
{
    HAL::Button_Info_t btnInfo;
    if (_dev->read(&btnInfo, sizeof(btnInfo)) != sizeof(btnInfo)) {
        return DataNode::RES_NO_DATA;
    }

    switch (param->event) {
    case DataNode::EVENT_TIMER:
        if (_waitRelease && btnInfo.key.ok) {
            /* wait for button release */
            return DataNode::RES_OK;
        }
        _waitRelease = false;

        _btnOK.monitor(btnInfo.key.ok);
        break;
    case DataNode::EVENT_PULL:
        if (param->size != sizeof(HAL::Button_Info_t)) {
            return DataNode::RES_SIZE_MISMATCH;
        }
        memcpy(param->data_p, &btnInfo, sizeof(btnInfo));
        break;
    default:
        return DataNode::RES_UNSUPPORTED_REQUEST;
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
    } else {
        return;
    }

    ctx->_node->publish(&info, sizeof(info));
}

DATA_PROC_DESCRIPTOR_DEF(Button)
