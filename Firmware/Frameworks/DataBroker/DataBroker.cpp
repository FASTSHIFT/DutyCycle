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
#include "DataBroker.h"
#include "DataBrokerLog.h"
#include "DataTimer.h"
#include <algorithm>
#include <string.h>

DataBroker::DataBroker()
    : _nodePool(nullptr)
    , _timerManager(nullptr)
{
    _nodePool = new DataNode::DataNodeList_t;
}

DataBroker::~DataBroker()
{
    DN_LOG_WARN("DataBroker closing...");

    /* check leak */
    for (auto iter : *_nodePool) {
        (void)iter;
        DN_LOG_WARN("Leak: DataNode[%s]", iter->getID());
    }

    DN_LOG_INFO("Deleting node pool: %p", _nodePool);
    delete _nodePool;
    _nodePool = nullptr;

    if (_timerManager) {
        DN_LOG_INFO("Deleting timer manager: %p", _timerManager);
        delete _timerManager;
        _timerManager = nullptr;
    }

    DN_LOG_WARN("DataBroker closed.");
}

DataNode* DataBroker::search(const char* id)
{
    return search(_nodePool, id);
}

DataNode* DataBroker::search(DataNode::DataNodeList_t* vec, const char* id)
{
    for (auto iter : *vec) {
        if (strcmp(id, iter->getID()) == 0) {
            return iter;
        }
    }
    return nullptr;
}

bool DataBroker::add(DataNode* node)
{
    if (search(node->getID()) != nullptr) {
        DN_LOG_ERROR("Multi add DataNode[%s]", node->getID());
        return false;
    }

    _nodePool->push_back(node);
    return true;
}

bool DataBroker::remove(DataNode* node)
{
    return remove(_nodePool, node);
}

bool DataBroker::remove(DataNode::DataNodeList_t* vec, DataNode* node)
{
    if (!node) {
        DN_LOG_ERROR("DataNode is nullptr");
        return false;
    }

    auto iter = std::find(vec->begin(), vec->end(), node);

    if (iter == vec->end()) {
        DN_LOG_ERROR("DataNode[%s] was not found", node->getID());
        return false;
    }

    vec->erase(iter);

    return true;
}

size_t DataBroker::getNodeNumber()
{
    return _nodePool->size();
}

void DataBroker::initTimerManager(uint32_t (*tickFunction)(void))
{
    if (!_timerManager) {
        _timerManager = new DataTimerManager;
    }

    _timerManager->setTickCallback(tickFunction);
}

uint32_t DataBroker::handleTimer()
{
    return _timerManager->handler();
}
