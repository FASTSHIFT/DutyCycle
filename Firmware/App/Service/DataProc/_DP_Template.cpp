/*
 * MIT License
 * Copyright (c) 2021 - 2025 _VIFEXTech
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

class DP_Template {
public:
    DP_Template(DataNode* node);

private:
    DataNode* _node;
    int _value;

private:
    int onEvent(DataNode::EventParam_t* param);
};

DP_Template::DP_Template(DataNode* node)
    : _node(node)
    , _value(0)
{
    _node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_Template*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_PULL | DataNode::EVENT_NOTIFY);
}

int DP_Template::onEvent(DataNode::EventParam_t* param)
{
    if (param->size != sizeof(_value)) {
        return DataNode::RES_SIZE_MISMATCH;
    }

    switch (param->event) {
    case DataNode::EVENT_PULL:
        memcpy(param->data_p, &_value, sizeof(_value));
        break;

    case DataNode::EVENT_NOTIFY:
        memcpy(&_value, param->data_p, sizeof(_value));
        break;

    default:
        break;
    }

    return DataNode::RES_OK;
}

DATA_PROC_DESCRIPTOR_DEF(Template)
