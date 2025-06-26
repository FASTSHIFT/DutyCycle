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
#include <Arduino.h>

namespace HAL {

class Power : private DeviceObject {
public:
    Power(const char* name)
        : DeviceObject(name)
    {
    }

private:
    virtual int onInit();
    virtual int onIoctl(DeviceObject::IO_Cmd_t cmd, void* data);
    void setEnable(bool enable);
    void goToISP();
};

int Power::onInit()
{
    pinMode(CONFIG_PWR_EN_PIN, OUTPUT);

    setEnable(true);
    return DeviceObject::RES_OK;
}

int Power::onIoctl(DeviceObject::IO_Cmd_t cmd, void* data)
{
    switch (cmd.full) {
    case POWER_IOCMD_WFI:
        __WFI();
        break;

    case POWER_IOCMD_POWER_OFF:
        HAL_LOG_WARN("Power off!");
        setEnable(false);
        break;

    case POWER_IOCMD_REBOOT:
        HAL_LOG_WARN("Rebooting...");
        NVIC_SystemReset();
        break;

    case POWER_IOCMD_GOTO_ISP:
        goToISP();
        break;

    default:
        return DeviceObject::RES_UNSUPPORT;
    }
    return DeviceObject::RES_OK;
}

void Power::setEnable(bool enable)
{
    digitalWrite(CONFIG_PWR_EN_PIN, enable);
}

void Power::goToISP()
{
#define ISP_STACK_ADDRESS (0x1FFFE400)

    uint32_t JumpAddress = *(__IO uint32_t*)(ISP_STACK_ADDRESS + 4u);
    crm_reset();
    __disable_irq();

    SCB->VTOR = ISP_STACK_ADDRESS;
    __set_MSP((*(__IO uint32_t*)ISP_STACK_ADDRESS));
    __enable_irq();

    ((void (*)(void))(JumpAddress))();
}

} /* namespace HAL */

DEVICE_OBJECT_MAKE(Power);
