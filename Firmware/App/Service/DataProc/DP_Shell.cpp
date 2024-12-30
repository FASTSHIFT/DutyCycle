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
#include "Utils/Shell/Shell.h"

using namespace DataProc;

class DP_Shell {
public:
    DP_Shell(DataNode* node);

private:
    static DataNode* _node;
    const DataNode* _nodeGlobal;
    static DeviceObject* _devSerial;

private:
    int onEvent(DataNode::EventParam_t* param);
    void onGlobalEvent(const Global_Info_t* info);
    static int shellReader(char* data);
    static void shellWriter(char data);
    static uint32_t shellTickGet();
    static int cmdPublich(int argc, char** argv);
};

DataNode* DP_Shell::_node = nullptr;
DeviceObject* DP_Shell::_devSerial = nullptr;

DP_Shell::DP_Shell(DataNode* node)
{
    _node = node;
    _devSerial = HAL::Manager()->getDevice("SerialIO");
    if (!_devSerial) {
        return;
    }

    _nodeGlobal = _node->subscribe("Global");
    if (!_nodeGlobal) {
        return;
    }

    _node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_Shell*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_PUBLISH);

    shell_init(shellReader, shellWriter, shellTickGet, nullptr, nullptr);
    shell_register(cmdPublich, "publish");
}

int DP_Shell::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {
    case DataNode::EVENT_PUBLISH: {
        if (param->tran == _nodeGlobal) {
            onGlobalEvent((const Global_Info_t*)param->data_p);
        }
    } break;

    default:
        break;
    }

    return DataNode::RES_OK;
}

void DP_Shell::onGlobalEvent(const Global_Info_t* info)
{
    if (info->event == GLOBAL_EVENT::APP_RUN_LOOP_BEGIN) {
        shell_task();
    }
}

int DP_Shell::shellReader(char* data)
{
    return DP_Shell::_devSerial->read(data, sizeof(char));
}

void DP_Shell::shellWriter(char data)
{
    DP_Shell::_devSerial->write(&data, sizeof(data));
}

uint32_t DP_Shell::shellTickGet()
{
    return HAL::GetTick();
}

int DP_Shell::cmdPublich(int argc, char** argv)
{
    Shell_Info_t info;
    info.argc = argc;
    info.argv = argv;
    const int retval = DP_Shell::_node->publish(&info, sizeof(info));
    return retval == DataNode::RES_OK ? SHELL_RET_SUCCESS : SHELL_RET_FAILURE;
}

DATA_PROC_DESCRIPTOR_DEF(Shell)
