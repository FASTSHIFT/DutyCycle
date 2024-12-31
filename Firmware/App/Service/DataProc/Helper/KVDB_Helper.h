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
#ifndef __DATA_PROC_KVDB_HELPER_H
#define __DATA_PROC_KVDB_HELPER_H

#include <stdint.h>

class DataNode;

namespace DataProc {

class KVDB_Helper {
public:
    KVDB_Helper(DataNode* node);
    operator const DataNode*() const
    {
        return _nodeKVDB;
    }

    int set(const char* key, const char* value);
    int set(const char* key, const void* value, uint32_t size);
    int get(const char* key, void* value, uint32_t size);
    const char* get(const char* key);
    int remove(const char* key);

private:
    DataNode* _node;
    const DataNode* _nodeKVDB;
};

} // namespace DataProc

#endif // __DATA_PROC_KVDB_HELPER_H
