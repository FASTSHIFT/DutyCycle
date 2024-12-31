/*
 * MIT License
 * Copyright (c) 2023 - 2024 _VIFEXTech
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
#include "HAL.h"
#include "at32f421_flash.h"

#define FLASH_ADDR 0x08000000
#define FLASH_SIZE (64 * 1024)
#define FLASH_BLOCK_SIZE (1 * 1024)

namespace HAL {

class Flash : private DeviceObject {
public:
    Flash(const char* name)
        : DeviceObject(name)
        , _info {
            (uint8_t*)(uintptr_t)FLASH_ADDR,
            FLASH_SIZE,
            FLASH_BLOCK_SIZE
        }
       , _offset(0)
    {
    }

private:
    const HAL::Flash_Info_t _info;
    long _offset;

private:
    virtual int onInit();
    virtual int onRead(void* buffer, size_t size);
    virtual int onWrite(const void* buffer, size_t size);
    virtual int onIoctl(DeviceObject::IO_Cmd_t cmd, void* data);
};

int Flash::onInit()
{
    return DeviceObject::RES_OK;
}

int Flash::onRead(void* buffer, size_t size)
{
    memcpy(buffer, (const uint8_t*)_info.addr + _offset, size);
    return size;
}

int Flash::onWrite(const void* buffer, size_t size)
{
    size_t dest = (uintptr_t)_info.addr + _offset;
    const uint8_t* src = (const uint8_t*)buffer;
    for (size_t i = 0; i < size; i++) {
        flash_byte_program(dest, *src);
        dest++;
        src++;
    }

    return size;
}

int Flash::onIoctl(DeviceObject::IO_Cmd_t cmd, void* data)
{
    switch (cmd.full) {
    case FLASH_IOCMD_LOCK:
        flash_lock();
        break;

    case FLASH_IOCMD_UNLOCK:
        flash_unlock();
        break;

    case FLASH_IOCMD_ERASE: {
        size_t addr = (uintptr_t)_info.addr + _offset;
        size_t size = *(size_t*)data;
        for (size_t blk_addr = addr; blk_addr < addr + size; blk_addr += _info.blk_size) {
            flash_sector_erase(blk_addr);
        }
    } break;

    case FLASH_IOCMD_GET_INFO:
        memcpy(data, &_info, sizeof(_info));
        break;

    case FLASH_IOCMD_SET_OFFSET:
        _offset = *(long*)data;
        break;

    case FLASH_IOCMD_SAVE:
        break;

    default:
        return DeviceObject::RES_UNSUPPORT;
    }
    return DeviceObject::RES_OK;
}

} /* namespace HAL */

DEVICE_OBJECT_MAKE(Flash);
