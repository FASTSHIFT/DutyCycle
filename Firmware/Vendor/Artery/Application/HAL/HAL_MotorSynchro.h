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
#ifndef __HAL_MOTOR_SYNCHRO_H
#define __HAL_MOTOR_SYNCHRO_H

#include "HAL.h"
#include "Utils/synchro/synchro.h"

namespace HAL {

#define MOTOR_SYNCHRO_HEAD_COUNT 2

/**
 * Synchro backend of the "Motor" device.
 *
 * It keeps the Motor_Info_t contract of the PWM meters, so DP_Ctrl and
 * MotorCtrl are untouched: value[0] drives head A, value[1] drives head B,
 * a negative value means "leave this head alone".
 *
 * A motor value 0..1000 is one electrical turn, 0..360 degrees, and that angle
 * is turned into three duties by the synchro library. The scale of the dial
 * (where it starts and ends) is not known here: it is whatever motor values
 * the host calibrates through the clock map and the motor_min/motor_max of the
 * web tool, exactly as with the PWM meters.
 *
 * Nothing runs from an interrupt: the duties are recomputed only when a new
 * value arrives, and the compare registers are preloaded so a write never
 * lands in the middle of a carrier period.
 *
 * The timers are only claimed between start() and stop().
 */
class MotorSynchro {
public:
    MotorSynchro();

    /** Claim the pins and timers. Returns false, with everything released, on failure. */
    bool start();

    /** Release the pins and timers, the outputs float. */
    void stop();

    void write(const Motor_Info_t* info);
    void read(Motor_Info_t* info) const;

private:
    typedef struct
    {
        synchro_t synchro;
        int16_t value;
    } Head_t;

private:
    Head_t _head[MOTOR_SYNCHRO_HEAD_COUNT];

private:
    static const uint8_t* getPins(int id);
    void update(int id);
};

} /* namespace HAL */

#endif /* __HAL_MOTOR_SYNCHRO_H */
