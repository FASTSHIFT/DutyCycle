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
#include "HAL_MotorSynchro.h"

#define MOTOR_VALUE_FULL 1000

/**
 * Phase per 0.1 degree, round(2^32 / 3600).
 * 0.1 degree is finer than what the duty quantisation can resolve, and the
 * product with the largest angle (3599) still fits in 32 bit.
 */
#define PHASE_PER_DECI_DEG 1193046u
#define DECI_DEG_PER_TURN 3600u

namespace HAL {

MotorSynchro::MotorSynchro()
{
    for (auto& h : _head) {
        h.value = 0;
    }
}

const uint8_t* MotorSynchro::getPins(int id)
{
    static const uint8_t pins[MOTOR_SYNCHRO_HEAD_COUNT][SYNCHRO_PHASES] = {
        { CONFIG_SYNCHRO_A_U_PIN, CONFIG_SYNCHRO_A_V_PIN, CONFIG_SYNCHRO_A_W_PIN },
        { CONFIG_SYNCHRO_B_U_PIN, CONFIG_SYNCHRO_B_V_PIN, CONFIG_SYNCHRO_B_W_PIN },
    };
    return pins[id];
}

bool MotorSynchro::start()
{
    for (int id = 0; id < MOTOR_SYNCHRO_HEAD_COUNT; id++) {
        const uint8_t* pins = getPins(id);

        for (int i = 0; i < SYNCHRO_PHASES; i++) {
            /* Fails when the pin has no timer or SYNCHRO_RES * carrier exceeds the timer clock */
            if (!IS_PWM_PIN(pins[i]) || PWM_Init(pins[i], SYNCHRO_RES, CONFIG_SYNCHRO_CARRIER_HZ) == 0) {
                stop();
                return false;
            }
        }
    }

    for (int id = 0; id < MOTOR_SYNCHRO_HEAD_COUNT; id++) {
        const uint8_t* pins = getPins(id);

        for (int i = 0; i < SYNCHRO_PHASES; i++) {
            PWM_SetPreload(pins[i], true);
        }

        synchro_init(&_head[id].synchro, CONFIG_SYNCHRO_AMPLITUDE);
        _head[id].value = 0;
        update(id);
    }

    return true;
}

void MotorSynchro::stop()
{
    for (int id = 0; id < MOTOR_SYNCHRO_HEAD_COUNT; id++) {
        const uint8_t* pins = getPins(id);

        for (int i = 0; i < SYNCHRO_PHASES; i++) {
            PWM_Deinit(pins[i]);
        }
    }
}

void MotorSynchro::update(int id)
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

void MotorSynchro::write(const Motor_Info_t* info)
{
    for (int id = 0; id < MOTOR_SYNCHRO_HEAD_COUNT; id++) {
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
}

void MotorSynchro::read(Motor_Info_t* info) const
{
    for (int id = 0; id < MOTOR_SYNCHRO_HEAD_COUNT; id++) {
        info->value[id] = _head[id].value;
    }
}

} /* namespace HAL */
