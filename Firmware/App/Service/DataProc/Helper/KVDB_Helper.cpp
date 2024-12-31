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
#include "KVDB_Helper.h"
#include "../Def/KVDB.h"
#include "Frameworks/DataBroker/DataBroker.h"

using namespace DataProc;

KVDB_Helper::KVDB_Helper(DataNode* node)
    : _node(node)
{
    _nodeKVDB = node->subscribe("KVDB");
}

int KVDB_Helper::set(const char* key, const char* value)
{
    KVDB_Info_t info;
    info.cmd = KVDB_CMD::SET;
    info.key = key;
    info.value = value;
    return _node->notify(_nodeKVDB, &info, sizeof(info));
}

int KVDB_Helper::set(const char* key, const void* value, uint32_t size)
{
    KVDB_Info_t info;
    info.cmd = KVDB_CMD::SET_BLOB;
    info.key = key;
    info.value = value;
    info.size = size;
    return _node->notify(_nodeKVDB, &info, sizeof(info));
}

int KVDB_Helper::get(const char* key, void* value, uint32_t size)
{
    KVDB_Info_t info;
    info.cmd = KVDB_CMD::GET;
    info.key = key;
    info.value = value;
    info.size = size;
    return _node->pull(_nodeKVDB, &info, sizeof(info));
}

const char* KVDB_Helper::get(const char* key)
{
    KVDB_Info_t info;
    info.cmd = KVDB_CMD::GET;
    info.key = key;
    _node->pull(_nodeKVDB, &info, sizeof(info));
    return (const char*)info.value;
}

int KVDB_Helper::remove(const char* key)
{
    KVDB_Info_t info;
    info.cmd = KVDB_CMD::DEL;
    info.key = key;
    return _node->notify(_nodeKVDB, &info, sizeof(info));
}
