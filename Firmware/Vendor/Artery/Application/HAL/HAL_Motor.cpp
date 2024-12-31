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

namespace HAL {

class Motor : private DeviceObject {
public:
    Motor(const char* name)
        : DeviceObject(name)
    {
    }

private:
    virtual int onInit();
    virtual int onWrite(const void* buffer, size_t size);
    virtual int onRead(void* buffer, size_t size);
    void motorWrite(int value);
    int motorRead();
    int readPWM(uint8_t pin);
};

int Motor::onInit()
{
    pinMode(CONFIG_MOTOR_OUT1_PIN, PWM);
    pinMode(CONFIG_MOTOR_OUT2_PIN, PWM);
    return DeviceObject::RES_OK;
}

int Motor::onWrite(const void* buffer, size_t size)
{
    if (size != sizeof(int)) {
        return DeviceObject::RES_PARAM_ERROR;
    }

    motorWrite(*((int*)buffer));
    return sizeof(int);
}

int Motor::onRead(void* buffer, size_t size)
{
    if (size != sizeof(int)) {
        return DeviceObject::RES_PARAM_ERROR;
    }

    *((int*)buffer) = motorRead();
    return sizeof(int);
}

void Motor::motorWrite(int value)
{
    analogWrite(CONFIG_MOTOR_OUT1_PIN, value > 0 ? value : 0);
    analogWrite(CONFIG_MOTOR_OUT2_PIN, value < 0 ? (uint16_t)-value : 0);
}

int Motor::motorRead()
{
    int value = readPWM(CONFIG_MOTOR_OUT1_PIN);
    if (value > 0) {
        return value;
    }

    value = readPWM(CONFIG_MOTOR_OUT2_PIN);
    return -value;
}

int Motor::readPWM(uint8_t pin)
{
    return Timer_GetCompare(PIN_MAP[pin].TIMx, PIN_MAP[pin].TimerChannel);
}

} /* namespace HAL */

DEVICE_OBJECT_MAKE(Motor);
