/*
 * MIT License
 * Copyright (c) 2024 _VIFEXTech
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

#include "App.h"
#include "HAL/HAL.h"
#include "Service/DataProc/DataProc.h"

struct AppContext {
    DataBroker* broker;
    DataProc::Global_Helper* global;
};

AppContext_t* App_CreateContext(int argc, const char* argv[])
{
    AppContext_t* context = new AppContext_t;

    /* HAL */
    HAL::Init();

    /* Data processor */
    context->broker = new DataBroker("Broker");
    DataProc_Init(context->broker);

    context->global = new DataProc::Global_Helper(context->broker->mainNode());
    context->global->publish(DataProc::GLOBAL_EVENT::DATA_PROC_INIT_FINISHED);

    /* App started */
    context->global->publish(DataProc::GLOBAL_EVENT::APP_STARTED);

    HAL_MemoryDumpInfo();

    return context;
}

uint32_t App_RunLoopExecute(AppContext_t* context)
{
    context->global->publish(DataProc::GLOBAL_EVENT::APP_RUN_LOOP_BEGIN);
    uint32_t timeTillNext = context->broker->handleTimer();
    context->global->publish(DataProc::GLOBAL_EVENT::APP_RUN_LOOP_END, &timeTillNext);
//    HAL_LOG_INFO("timeTillNext = %d", timeTillNext);
    return timeTillNext;
}

void App_DestroyContext(AppContext_t* context)
{
    context->global->publish(DataProc::GLOBAL_EVENT::APP_STOPPED);

    delete context->broker;
    delete context;
}
