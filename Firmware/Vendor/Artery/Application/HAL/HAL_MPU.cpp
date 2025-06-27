/*
 * MIT License
 * Copyright (c) 2023 - 2025 _VIFEXTech
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

#if defined(__ARMCC_VERSION)
#define CSTACK_BLOCK_NAME STACK

#define SECTION_START(_name_) _name_##$$Base
#define SECTION_END(_name_) _name_##$$Limit

#define CSTACK_BLOCK_START(_name_) SECTION_START(_name_)
#define CSTACK_BLOCK_END(_name_) SECTION_END(_name_)

#define CSTACK_START CSTACK_BLOCK_START(CSTACK_BLOCK_NAME)
#define CSTACK_END CSTACK_BLOCK_END(CSTACK_BLOCK_NAME)

extern const int CSTACK_START;
extern const int CSTACK_END;

#elif defined(__GNUC__)

extern const int _sstack;
extern const int _estack;

#define CSTACK_START _sstack
#define CSTACK_END _estack

#else
#error "Unsupported compiler"
#endif

/* Protection Stack Size, set to 0 to disable */
#define PROTECTION_STACK_CHECK_SIZE 0

/* Protection Region Size */
#define PROTECTION_REGION_SIZE (64)

/* C standard library functions (such as vsnprintf) may dynamically
 * allocate temporary buffers on the heap and add an offset to the MPU protection zone address
 * to allow a small amount of use of the heap
 */
#define REGION_OFFSET (64)

/* Protected Address */
#define REGION_ADDRESS ((uintptr_t) & CSTACK_START)

/* Region Size */
#define REGION_SIZE ARM_MPU_REGION_SIZE_64B

/* Number(16bit) */
#define REGION_REGION_NUMBER 0

/* Access Permission */
#define REGION_PERMISSION ARM_MPU_AP_RO

void HAL_MPU_Init()
{
#if PROTECTION_STACK_CHECK_SIZE
    uint32_t stack_size = (uint32_t)&CSTACK_END - (uint32_t)&CSTACK_START;
    /* stack size check */
    if (stack_size < PROTECTION_STACK_CHECK_SIZE) {
        HAL_LOG_ERROR("Stack size is too small: %" PRIu32 " Bytes (< %" PRIu32 ")",
            stack_size, PROTECTION_STACK_CHECK_SIZE);
    }
#endif

    ARM_MPU_Disable();

    uint32_t region = REGION_REGION_NUMBER;
    uint32_t addr = REGION_ADDRESS + REGION_OFFSET;
    uint32_t rbar = ARM_MPU_RBAR(region, addr);

    uint32_t disableExec = 0;
    uint32_t accessPermission = REGION_PERMISSION;
    uint32_t typeExtField = 0;
    uint32_t isShareable = 0;
    uint32_t isCacheable = 0;
    uint32_t isBufferable = 0;
    uint32_t subRegionDisable = 0;
    uint32_t size = REGION_SIZE;
    uint32_t rasr = ARM_MPU_RASR(
        disableExec,
        accessPermission,
        typeExtField,
        isShareable,
        isCacheable,
        isBufferable,
        subRegionDisable,
        size);

    ARM_MPU_SetRegion(rbar, rasr);

    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
}

extern "C" void MemManage_Handler(void)
{
    uint32_t ctrl = __get_CONTROL();
    uint32_t cfsr = SCB->CFSR;

    HAL_LOG_ERROR("CTRL 0x%x", ctrl);
    HAL_LOG_ERROR("CPU ID 0x%x", SCB->CPUID);
    HAL_LOG_ERROR("MemManage Fault Address 0x%x", SCB->MMFAR);

    if (cfsr & SCB_CFSR_MMARVALID_Msk) {
        HAL_LOG_ERROR("Attempt to access address");
    }

    if (cfsr & SCB_CFSR_DACCVIOL_Msk) {
        HAL_LOG_ERROR("Operation not permitted");
    }

    if (cfsr & SCB_CFSR_IACCVIOL_Msk) {
        HAL_LOG_ERROR("Non-executable region");
    }

    if (cfsr & SCB_CFSR_MSTKERR_Msk) {
        HAL_LOG_ERROR("Stacking error");
    }

    /* Disable MPU and restart instruction */
    ARM_MPU_Disable();

    NVIC_SystemReset();
}
