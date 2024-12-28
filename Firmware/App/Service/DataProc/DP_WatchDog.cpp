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
#include "HAL/HAL_Config.h"
#include "Service/HAL/HAL.h"

using namespace DataProc;

class DP_WatchDog {
public:
    DP_WatchDog(DataNode* node);

private:
    DeviceObject* _dev;

private:
    int onEvent(DataNode::EventParam_t* param);
    void onWatchDogTimeout();
};

DP_WatchDog::DP_WatchDog(DataNode* node)
{
    _dev = HAL::Manager()->getDevice("WatchDog");
    if (!_dev) {
        return;
    }

    node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_WatchDog*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_TIMER);

#if CONFIG_WATCHDOG_TIMEOUT > 0
    int timeout = CONFIG_WATCHDOG_TIMEOUT * 1000;
    _dev->ioctl(WATCHDOG_IOCMD_SET_TIMEOUT, &timeout, sizeof(timeout));

    HAL::WatchDog_Callback_t callback = {
        [](void* arg) {
            auto ctx = (DP_WatchDog*)arg;
            ctx->onWatchDogTimeout();
        },
        this
    };
    _dev->ioctl(WATCHDOG_IOCMD_SET_CALLBACK, &callback, sizeof(callback));

    _dev->ioctl(WATCHDOG_IOCMD_ENABLE);
    node->startTimer(timeout / 2);
#endif
}

int DP_WatchDog::onEvent(DataNode::EventParam_t* param)
{
    if (_dev->ioctl(WATCHDOG_IOCMD_KEEP_ALIVE) != DeviceObject::RES_OK) {
        return DataNode::RES_UNKNOWN;
    }

    return DataNode::RES_OK;
}

void DP_WatchDog::onWatchDogTimeout()
{
}

DATA_PROC_DESCRIPTOR_DEF(WatchDog)
