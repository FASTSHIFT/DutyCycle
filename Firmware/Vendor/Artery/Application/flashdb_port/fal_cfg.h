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
#ifndef __FAL_CFG_H__
#define __FAL_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

void HAL_Log_Printf(const char* fmt, ...);
#define FAL_PRINTF(...) HAL_Log_Printf(__VA_ARGS__)

#ifndef FAL_DEBUG
#define FAL_DEBUG 0
#endif
#define FAL_PART_HAS_TABLE_CFG

/* ===================== Flash device Configuration ========================= */
extern struct fal_flash_dev dev_onchip_flash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE \
    {                       \
        &dev_onchip_flash,  \
    }

#define FDB_KVDB_PART_NAME "fdb_kvdb"

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                                               \
    {                                                                                \
        { FAL_PART_MAGIC_WORD, FDB_KVDB_PART_NAME, "onchip", (64 - 1) * 1024, 1024, 0 }, \
    }
#endif /* FAL_PART_HAS_TABLE_CFG */

#ifdef __cplusplus
}
#endif

#endif
