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
#include "HAL_MotorSynchro.h"

namespace HAL {

/**
 * Motor device with two selectable backends:
 *  - MOTOR_MODE_PWM: two PWM meters on CONFIG_MOTOR_OUT1/2_PIN (default)
 *  - MOTOR_MODE_SYNCHRO: two synchro heads, see MotorSynchro
 *
 * Only the pins and timers of the active mode are claimed. Selecting another
 * mode releases the old ones first, so the timers a mode needs stay switched
 * off (and their clocks gated) while it is not in use.
 */
class Motor : private DeviceObject {
public:
    Motor(const char* name)
        : DeviceObject(name)
        , _mode(MOTOR_MODE_PWM)
    {
    }

private:
    Motor_Mode_t _mode;
    MotorSynchro _synchro;

private:
    virtual int onInit();
    virtual int onWrite(const void* buffer, size_t size);
    virtual int onRead(void* buffer, size_t size);
    virtual int onIoctl(DeviceObject::IO_Cmd_t cmd, void* data);

    bool startMode(Motor_Mode_t mode);
    void stopMode(Motor_Mode_t mode);
    uint16_t readPWM(uint8_t pin);
};

int Motor::onInit()
{
    return startMode(_mode) ? DeviceObject::RES_OK : DeviceObject::RES_INIT_FAILED;
}

bool Motor::startMode(Motor_Mode_t mode)
{
    switch (mode) {
    case MOTOR_MODE_PWM:
        pinMode(CONFIG_MOTOR_OUT1_PIN, PWM);
        pinMode(CONFIG_MOTOR_OUT2_PIN, PWM);
        return true;

    case MOTOR_MODE_SYNCHRO:
        return _synchro.start();
    }

    return false;
}

void Motor::stopMode(Motor_Mode_t mode)
{
    switch (mode) {
    case MOTOR_MODE_PWM:
        PWM_Deinit(CONFIG_MOTOR_OUT1_PIN);
        PWM_Deinit(CONFIG_MOTOR_OUT2_PIN);
        break;

    case MOTOR_MODE_SYNCHRO:
        _synchro.stop();
        break;
    }
}

int Motor::onIoctl(DeviceObject::IO_Cmd_t cmd, void* data)
{
    switch (cmd.full) {
    case MOTOR_IOCMD_SET_MODE: {
        const Motor_Mode_t mode = *(const Motor_Mode_t*)data;

        if (mode != MOTOR_MODE_PWM && mode != MOTOR_MODE_SYNCHRO) {
            return DeviceObject::RES_PARAM_ERROR;
        }

        if (mode == _mode) {
            break;
        }

        stopMode(_mode);

        if (!startMode(mode)) {
            /* Never leave the meters without a driver */
            _mode = MOTOR_MODE_PWM;
            startMode(_mode);
            return DeviceObject::RES_INIT_FAILED;
        }

        _mode = mode;
    } break;

    default:
        return DeviceObject::RES_UNSUPPORT;
    }

    return DeviceObject::RES_OK;
}

int Motor::onWrite(const void* buffer, size_t size)
{
    if (size != sizeof(Motor_Info_t)) {
        return DeviceObject::RES_PARAM_ERROR;
    }

    auto info = (const Motor_Info_t*)buffer;

    if (_mode == MOTOR_MODE_SYNCHRO) {
        _synchro.write(info);
        return sizeof(Motor_Info_t);
    }

    if (info->value[0] >= 0) {
        analogWrite(CONFIG_MOTOR_OUT1_PIN, info->value[0]);
    }

    if (info->value[1] >= 0) {
        analogWrite(CONFIG_MOTOR_OUT2_PIN, info->value[1]);
    }

    return sizeof(Motor_Info_t);
}

int Motor::onRead(void* buffer, size_t size)
{
    if (size != sizeof(Motor_Info_t)) {
        return DeviceObject::RES_PARAM_ERROR;
    }

    auto info = (Motor_Info_t*)buffer;

    if (_mode == MOTOR_MODE_SYNCHRO) {
        _synchro.read(info);
        return sizeof(Motor_Info_t);
    }

    info->value[0] = readPWM(CONFIG_MOTOR_OUT1_PIN);
    info->value[1] = readPWM(CONFIG_MOTOR_OUT2_PIN);
    return sizeof(Motor_Info_t);
}

uint16_t Motor::readPWM(uint8_t pin)
{
    return Timer_GetCompare(PIN_MAP[pin].TIMx, PIN_MAP[pin].TimerChannel);
}

} /* namespace HAL */

DEVICE_OBJECT_MAKE(Motor);
