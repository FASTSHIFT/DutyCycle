/*
 * MIT License
 * Copyright (c) 2023 HanfG, _VIFEXTech
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
#include "HAL/HAL.h"
#include <fal.h>

static DeviceObject* devFlash = nullptr;

static int flash_init(void)
{
    devFlash = HAL::Manager()->getDevice("Flash");
    if (!devFlash) {
        return 0;
    }

    HAL::Flash_Info_t info;
    int ret = devFlash->ioctl(FLASH_IOCMD_GET_INFO, &info, sizeof(HAL::Flash_Info_t));
    if (ret != DeviceObject::RES_OK) {
        return 0;
    }

    dev_onchip_flash.addr = (uint32_t)(uintptr_t)info.addr;
    dev_onchip_flash.len = info.len;
    dev_onchip_flash.blk_size = info.blk_size;
    return 1;
}

static int flash_read(long offset, uint8_t* buf, size_t size)
{
    devFlash->ioctl(FLASH_IOCMD_SET_OFFSET, &offset, sizeof(long));
    return devFlash->read(buf, size);
}

static int flash_write(long offset, const uint8_t* buf, size_t size)
{
    devFlash->ioctl(FLASH_IOCMD_UNLOCK);
    devFlash->ioctl(FLASH_IOCMD_SET_OFFSET, &offset, sizeof(long));
    size = devFlash->write(buf, size);
    devFlash->ioctl(FLASH_IOCMD_LOCK);
    return size;
}

static int flash_erase(long offset, size_t size)
{
    devFlash->ioctl(FLASH_IOCMD_UNLOCK);
    devFlash->ioctl(FLASH_IOCMD_SET_OFFSET, &offset, sizeof(long));
    devFlash->ioctl(FLASH_IOCMD_ERASE, &size, sizeof(size_t));
    devFlash->ioctl(FLASH_IOCMD_LOCK);
    return size;
}

struct fal_flash_dev dev_onchip_flash = {
    "onchip", /* name */
    0, /* addr */
    0, /* len */
    0, /* blk_size */
    { flash_init, flash_read, flash_write, flash_erase }, /* ops */
    8 /* write_gran */
};
