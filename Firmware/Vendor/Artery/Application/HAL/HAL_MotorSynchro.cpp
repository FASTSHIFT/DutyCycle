/*
 * MIT License
 * Copyright (c) 2026 _VIFEXTech
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

#if CONFIG_MOTOR_DRIVER_SYNCHRO

#include "Utils/synchro/synchro.h"

/**
 * Synchro backend of the "Motor" device.
 *
 * It keeps the Motor_Info_t contract of the PWM driver, so DP_Ctrl and
 * MotorCtrl are untouched: value[0] drives head A, value[1] drives head B,
 * a negative value means "leave this head alone".
 *
 * A motor value 0..1000 is one electrical turn, 0..360 degrees, and that angle
 * is turned into three duties by the synchro library. The scale of the dial
 * (where it starts and ends) is not known here: it is whatever motor values
 * the host calibrates through the clock map and the motor_min/motor_max of the
 * web tool, exactly as with the PWM driver.
 *
 * Nothing runs from an interrupt: the duties are recomputed only when a new
 * value arrives, and the compare registers are preloaded so a write never
 * lands in the middle of a carrier period.
 */

#define MOTOR_HEAD_COUNT 2
#define MOTOR_VALUE_FULL 1000

/**
 * Phase per 0.1 degree, round(2^32 / 3600).
 * 0.1 degree is finer than what the duty quantisation can resolve, and the
 * product with the largest angle (3599) still fits in 32 bit.
 */
#define PHASE_PER_DECI_DEG 1193046u
#define DECI_DEG_PER_TURN 3600u

namespace HAL {

class Motor : private DeviceObject {
public:
    Motor(const char* name)
        : DeviceObject(name)
    {
        for (auto& h : _head) {
            h.value = 0;
        }
    }

private:
    typedef struct
    {
        synchro_t synchro;
        int16_t value;
    } Head_t;

private:
    Head_t _head[MOTOR_HEAD_COUNT];

private:
    virtual int onInit();
    virtual int onWrite(const void* buffer, size_t size);
    virtual int onRead(void* buffer, size_t size);

    static const uint8_t* getPins(int id);
    void update(int id);
};

const uint8_t* Motor::getPins(int id)
{
    static const uint8_t pins[MOTOR_HEAD_COUNT][SYNCHRO_PHASES] = {
        { CONFIG_SYNCHRO_A_U_PIN, CONFIG_SYNCHRO_A_V_PIN, CONFIG_SYNCHRO_A_W_PIN },
        { CONFIG_SYNCHRO_B_U_PIN, CONFIG_SYNCHRO_B_V_PIN, CONFIG_SYNCHRO_B_W_PIN },
    };
    return pins[id];
}

int Motor::onInit()
{
    for (int id = 0; id < MOTOR_HEAD_COUNT; id++) {
        const uint8_t* pins = getPins(id);

        for (int i = 0; i < SYNCHRO_PHASES; i++) {
            /* Fails when the pin has no timer or SYNCHRO_RES * carrier exceeds the timer clock */
            if (!IS_PWM_PIN(pins[i]) || PWM_Init(pins[i], SYNCHRO_RES, CONFIG_SYNCHRO_CARRIER_HZ) == 0) {
                return DeviceObject::RES_INIT_FAILED;
            }
        }
    }

    for (int id = 0; id < MOTOR_HEAD_COUNT; id++) {
        const uint8_t* pins = getPins(id);

        for (int i = 0; i < SYNCHRO_PHASES; i++) {
            PWM_SetPreload(pins[i], true);
        }

        synchro_init(&_head[id].synchro, CONFIG_SYNCHRO_AMPLITUDE);
        update(id);
    }

    return DeviceObject::RES_OK;
}

void Motor::update(int id)
{
    Head_t& h = _head[id];

    /* value 0..1000 -> 0..3600 (0.1 degree), 1000 wraps back to 0 */
    const uint32_t angle = ((uint32_t)h.value * DECI_DEG_PER_TURN / MOTOR_VALUE_FULL) % DECI_DEG_PER_TURN;

    synchro_set_phase(&h.synchro, angle * PHASE_PER_DECI_DEG);
    synchro_compute(&h.synchro);

    const uint8_t* pins = getPins(id);
    for (int i = 0; i < SYNCHRO_PHASES; i++) {
        PWM_Write(pins[i], h.synchro.duty[i]);
    }
}

int Motor::onWrite(const void* buffer, size_t size)
{
    if (size != sizeof(Motor_Info_t)) {
        return DeviceObject::RES_PARAM_ERROR;
    }

    auto info = (const Motor_Info_t*)buffer;

    for (int id = 0; id < MOTOR_HEAD_COUNT; id++) {
        int value = info->value[id];

        if (value < 0) {
            continue;
        }

        if (value > MOTOR_VALUE_FULL) {
            value = MOTOR_VALUE_FULL;
        }

        _head[id].value = (int16_t)value;
        update(id);
    }

    return sizeof(Motor_Info_t);
}

int Motor::onRead(void* buffer, size_t size)
{
    if (size != sizeof(Motor_Info_t)) {
        return DeviceObject::RES_PARAM_ERROR;
    }

    auto info = (Motor_Info_t*)buffer;
    for (int id = 0; id < MOTOR_HEAD_COUNT; id++) {
        info->value[id] = _head[id].value;
    }

    return sizeof(Motor_Info_t);
}

} /* namespace HAL */

DEVICE_OBJECT_MAKE(Motor);

#endif /* CONFIG_MOTOR_DRIVER_SYNCHRO */
