/*
 * MIT License
 * Copyright (c) 2021 - 2023 _VIFEXTech
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
#include "../HAL/HAL_Log.h"
#include "DataProc.h"
#include "Version.h"

/* clang-format off */

/* Number to string macro */
#define _VERSION_NUM_TO_STR_(n)  #n
#define VERSION_NUM_TO_STR(n)   _VERSION_NUM_TO_STR_(n)

/* Compiler Version */
#if defined(_MSC_FULL_VER)
#  define VERSION_COMPILER      "MSVC v" VERSION_NUM_TO_STR(_MSC_FULL_VER)
#elif defined(__ARMCC_VERSION)
#  define VERSION_COMPILER      "ARMCC v" VERSION_NUM_TO_STR(__ARMCC_VERSION)
#elif defined(__GNUC__)
#  define VERSION_COMPILER      "GCC v"\
                                VERSION_NUM_TO_STR(__GNUC__)\
                                "."\
                                VERSION_NUM_TO_STR(__GNUC_MINOR__)\
                                "."\
                                VERSION_NUM_TO_STR(__GNUC_PATCHLEVEL__)
#else
#  define VERSION_COMPILER      "UNKNOW"
#endif

/* clang-format on */

using namespace DataProc;

class DP_Version {
public:
    DP_Version(DataNode* node);

private:
    int onEvent(DataNode::EventParam_t* param);
    void getInfo(Version_Info_t* info);
    void dumpInfo();
};

DP_Version::DP_Version(DataNode* node)
{
    node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_Version*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_PULL | DataNode::EVENT_NOTIFY);

    dumpInfo();
}

int DP_Version::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {
    case DataNode::EVENT_PULL: {
        if (param->size != sizeof(Version_Info_t)) {
            return DataNode::RES_SIZE_MISMATCH;
        }

        auto info = (Version_Info_t*)param->data_p;
        getInfo(info);
    } break;

    case DataNode::EVENT_NOTIFY:
        dumpInfo();
        break;

    default:
        return DataNode::RES_UNSUPPORTED_REQUEST;
    }

    return DataNode::RES_OK;
}

void DP_Version::getInfo(Version_Info_t* info)
{
    info->name = VERSION_FIRMWARE_NAME;
    info->software = VERSION_SOFTWARE;
    info->hardware = VERSION_HARDWARE;
    info->author = VERSION_AUTHOR_NAME;
    info->website = VERSION_WEBSITE;
    info->compiler = VERSION_COMPILER;
    info->buildDate = __DATE__;
    info->buildTime = __TIME__;

    extern void HAL_GetUID(HAL::UID_Info_t * info);
    HAL_GetUID(&info->uid);
}

void DP_Version::dumpInfo()
{
    Version_Info_t info;
    getInfo(&info);
    HAL_LOG_INFO("Firmware: %s", info.name);
    HAL_LOG_INFO("Software: %s", info.software);
    HAL_LOG_INFO("Hardware: %s", info.hardware);
    HAL_LOG_INFO("Author: %s", info.author);
    HAL_LOG_INFO("Website: %s", info.website);
    HAL_LOG_INFO("Compiler: %s", info.compiler);
    HAL_LOG_INFO("Build Time: %s %s", info.buildDate, info.buildTime);
    HAL_LOG_INFO("PID: 0x%08X", info.uid.pid);
    HAL_LOG_INFO("Flash Size: %d KB", info.uid.flashSize);
    HAL_LOG_INFO("UID: 0x%08X, 0x%08X, 0x%08X", info.uid.uid.u32[0], info.uid.uid.u32[1], info.uid.uid.u32[2]);
}

DATA_PROC_DESCRIPTOR_DEF(Version)
