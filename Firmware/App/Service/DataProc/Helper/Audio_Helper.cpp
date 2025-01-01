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
#include "Audio_Helper.h"
#include "Frameworks/DataBroker/DataBroker.h"

using namespace DataProc;

Audio_Helper::Audio_Helper(DataNode* node)
    : _node(node)
{
    _nodeAudio = node->subscribe("Audio");
}

int Audio_Helper::play(const Audio_Squence_t* squence, uint32_t length, uint32_t bpm, bool interruptible)
{
    Audio_Info_t info;
    info.squence = squence;
    info.length = length;
    info.interruptible = interruptible;
    if (bpm) {
        info.bpm = bpm;
    }
    return _node->notify(_nodeAudio, &info, sizeof(info));
}

int Audio_Helper::stop()
{
    Audio_Info_t info;
    info.squence = nullptr;
    info.length = 0;
    return _node->notify(_nodeAudio, &info, sizeof(info));
}
