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
#include "DataProc.h"
#include "HAL/HAL.h"
#include "fal_cfg.h"
#include "flashdb.h"

#define IS_STR_EQ(STR1, STR2) (strcmp(STR1, STR2) == 0)

using namespace DataProc;

class DP_KVDB {
public:
    DP_KVDB(DataNode* node);

private:
    struct fdb_kvdb _kvdb;
    DeviceObject* _dev;

private:
    int onEvent(DataNode::EventParam_t* param);
    int onSet(KVDB_Info_t* info);
    int onGet(KVDB_Info_t* info);
};

DP_KVDB::DP_KVDB(DataNode* node)
    : _dev(nullptr)
{
    _dev = HAL::Manager()->getDevice("Flash");
    if (!_dev) {
        return;
    }

    memset(&_kvdb, 0, sizeof(_kvdb));
    auto result = fdb_kvdb_init(&_kvdb, "config", FDB_KVDB_PART_NAME, nullptr, nullptr);
    if (result != FDB_NO_ERR) {
        HAL_LOG_ERROR("fdb_kvdb_init error: %d", result);
        return;
    }

    node->setEventCallback(
        [](DataNode* n, DataNode::EventParam_t* param) -> int {
            auto ctx = (DP_KVDB*)n->getUserData();
            return ctx->onEvent(param);
        },
        DataNode::EVENT_NOTIFY | DataNode::EVENT_NOTIFY | DataNode::EVENT_PUBLISH);
}

int DP_KVDB::onEvent(DataNode::EventParam_t* param)
{
    switch (param->event) {
    case DataNode::EVENT_NOTIFY:
        if (param->size != sizeof(KVDB_Info_t)) {
            return DataNode::RES_SIZE_MISMATCH;
        }
        return onSet((KVDB_Info_t*)param->data_p);

    case DataNode::EVENT_PULL:
        if (param->size != sizeof(KVDB_Info_t)) {
            return DataNode::RES_SIZE_MISMATCH;
        }
        return onGet((KVDB_Info_t*)param->data_p);

    default:
        break;
    }

    return DataNode::RES_OK;
}

int DP_KVDB::onSet(KVDB_Info_t* info)
{
    fdb_err_t err = FDB_NO_ERR;

    switch (info->cmd) {
    case KVDB_CMD::GET:
        onGet(info);
        break;
    case KVDB_CMD::SET:
        err = fdb_kv_set(&_kvdb, info->key, (const char*)info->value);
        break;
    case KVDB_CMD::SET_BLOB: {
        struct fdb_blob blob;
        err = fdb_kv_set_blob(&_kvdb, info->key, fdb_blob_make(&blob, info->value, info->size));
    } break;
    case KVDB_CMD::DEL:
        err = fdb_kv_del(&_kvdb, info->key);
        break;
    case KVDB_CMD::LIST: {
        struct fdb_kv_iterator iterator;
        fdb_kv_t cur_kv;

        fdb_kv_iterator_init(&_kvdb, &iterator);
        int index = 0;
        HAL_LOG_INFO("Key list:");
        while (fdb_kv_iterate(&_kvdb, &iterator)) {
            cur_kv = &(iterator.curr_kv);
            HAL_LOG_INFO("[%d]:", index);
            HAL_LOG_INFO("\tname = '%s'", cur_kv->name);
            HAL_LOG_INFO("\tstatus = %d", cur_kv->status);
            HAL_LOG_INFO("\tcrc_is_ok = %d", cur_kv->crc_is_ok);
            HAL_LOG_INFO("\tname_len = %d", cur_kv->name_len);
            HAL_LOG_INFO("\tmagic = 0x%" PRIu32, cur_kv->magic);
            HAL_LOG_INFO("\tlen = %d", cur_kv->len);
            HAL_LOG_INFO("\tvalue_len = %d", cur_kv->value_len);
            HAL_LOG_INFO("\taddr.start = 0x%" PRIu32, cur_kv->addr.start);
            HAL_LOG_INFO("\taddr.value = 0x%" PRIu32, cur_kv->addr.value);
            index++;
        }
    } break;
    case KVDB_CMD::SAVE:
        _dev->ioctl(FLASH_IOCMD_SAVE);
        break;
    default:
        break;
    }

    if (err != FDB_NO_ERR) {
        HAL_LOG_ERROR("kvdb error: %d", err);
    }

    return (err == FDB_NO_ERR) ? DataNode::RES_OK : DataNode::RES_NO_DATA;
}

int DP_KVDB::onGet(KVDB_Info_t* info)
{
    info->value = fdb_kv_get(&_kvdb, info->key);
    return DataNode::RES_OK;
}

DATA_PROC_DESCRIPTOR_DEF(KVDB)
