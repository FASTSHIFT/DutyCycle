/**
 * @file   synchro.h
 * @brief  Three-phase sine generator for a synchro or torque receiver.
 *
 * This module is pure algorithm. It includes nothing but stdint.h, keeps no
 * global state, and never touches pins, timers, PWM registers or interrupts.
 * Every instance lives in a caller-owned synchro_t, so the same code can drive
 * any number of gauges on any platform.
 *
 * The caller is responsible for two things:
 *   - calling synchro_update() at a steady SYNCHRO_HZ rate (a timer interrupt
 *     is the usual way), and
 *   - writing the resulting s->duty[] values to three PWM channels.
 *
 * What it computes is open-loop voltage vector modulation: a three-phase sine
 * set at an electrical angle that the module integrates itself. There is no
 * rotor position sensor and no current loop, so this is not field oriented
 * control. A synchro is a synchronous machine; it pulls itself into step with
 * the stator field, which is exactly what a gauge needs. Real FOC would need
 * the rotor angle, phase current sensing and a current loop on top.
 *
 * Everything is integer. The sine comes from a 66-entry quarter-wave table in
 * Q15 with quadrant folding and linear interpolation, which puts the table
 * itself at 2^-16 of a turn (0.0055 degrees) for 132 bytes of flash and no
 * libm. That is finer than the duty output, so SYNCHRO_RES, not the table, is
 * what limits the commanded angle in practice: at 14400 counts the output
 * lands within about 0.009 degrees of the angle asked for.
 *
 * Typical use. One object per gauge, named at the call site; an array would
 * only pay off if the number of gauges were not known at compile time:
 *
 *     static synchro_t gauge_a;
 *     static synchro_t gauge_b;
 *
 *     synchro_init(&gauge_a, 100);        // 100% amplitude
 *     synchro_init(&gauge_b, 100);
 *     synchro_set_speed(&gauge_a, 60);    // 60 electrical degrees per second
 *     synchro_set_speed(&gauge_b, -60);   // the other one counter-rotates
 *
 *     // from a timer interrupt running at SYNCHRO_HZ:
 *     synchro_update(&gauge_a);
 *     synchro_update(&gauge_b);
 *
 *     // gauge_a.duty[0..2] and gauge_b.duty[0..2] now hold the three phase
 *     // duties of each gauge. Pushing those to PWM channels is caller code:
 *     // this module never touches a pin, a timer or a register.
 */

#ifndef SYNCHRO_H
#define SYNCHRO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Number of output phases.
 * @note   Fixed by the physics, do not change.
 */
#define SYNCHRO_PHASES 3

/**
 * @brief  Upper bound of the duty values, exclusive.
 *
 * Set this to the resolution of whatever PWM hardware the caller uses. It only
 * scales the output; the algorithm itself is resolution agnostic.
 *
 * 14400 is the largest count a 288 MHz timer can give at a 20 kHz carrier
 * (288 MHz / 14400 / 20 kHz = 1, exactly). At 600 the duty quantisation, not
 * the phase accumulator, would be what limits the electrical angle to about
 * 0.1 degrees; 14400 brings that down to about 0.009 degrees for free.
 */
#ifndef SYNCHRO_RES
#define SYNCHRO_RES 14400
#endif

/**
 * @brief  Rate at which synchro_update() is expected to be called, in Hz.
 *
 * Used only to turn a requested speed in degrees per second into a phase step.
 * It must match the caller's timer, otherwise speeds are scaled by the
 * mismatch.
 */
#ifndef SYNCHRO_HZ
#define SYNCHRO_HZ 2000
#endif

/**
 * @brief  Overmodulation in percent of the undistorted sine reference.
 *
 *   100  pure sine, full linear range, lowest distortion (default)
 *   150  flat topped, roughly 15% more fundamental, better breakaway torque
 *   250+ degenerates into six-step
 *
 * At 100 the outputs already use the whole linear range of the bus, because
 * the common-mode injection gain is compensated for. Above 100 the waveform
 * is deliberately clipped.
 */
#ifndef SYNCHRO_OVERMOD
#define SYNCHRO_OVERMOD 100
#endif

/**
 * @brief  One synchro drive channel.
 * @note   Treat every field as read-only from the caller side.
 */
typedef struct
{
    uint32_t phase; /**< Electrical angle, 2^32 = 360 degrees. */
    int32_t step; /**< Phase increment per update. */
    int16_t gain; /**< Duty swing at the current amplitude. */
    uint16_t duty[SYNCHRO_PHASES]; /**< Last output, 0 .. SYNCHRO_RES-1. */
} synchro_t;

/**
 * @brief  Reset the object to angle 0, stopped, at the given amplitude.
 * @param  s: Channel to initialize.
 * @param  amp_pct: Output amplitude in percent, 0..100.
 */
void synchro_init(synchro_t *s, uint8_t amp_pct);

/**
 * @brief  Set the electrical speed.
 * @param  s: Channel to configure.
 * @param  dps: Degrees per second; negative reverses the direction.
 * @note   Clamped to the representable range, about +/-1300 degrees per
 *          second. Use synchro_set_step() if that is not enough.
 */
void synchro_set_speed(synchro_t *s, int16_t dps);

/**
 * @brief  Set the raw phase increment applied by each synchro_update().
 * @param  s: Channel to configure.
 * @param  phase_inc: Phase increment; 2^32 means one revolution per update.
 */
void synchro_set_step(synchro_t *s, int32_t phase_inc);

/**
 * @brief  Set the output amplitude.
 * @param  s: Channel to configure.
 * @param  pct: Amplitude in percent, 0..100; values above 100 are clamped.
 */
void synchro_set_amplitude(synchro_t *s, uint8_t pct);

/**
 * @brief  Jump to an absolute phase.
 * @param  s: Channel to configure.
 * @param  phase: Electrical angle, 2^32 = 360 degrees.
 * @note   Use together with synchro_set_speed(s, 0) for position control.
 */
void synchro_set_phase(synchro_t *s, uint32_t phase);

/**
 * @brief  Jump to an absolute electrical angle in whole degrees.
 * @param  s: Channel to configure.
 * @param  deg: Angle in degrees, 0..359; larger values wrap.
 */
void synchro_set_angle(synchro_t *s, uint16_t deg);

/**
 * @brief  Advance the phase by one step and refresh duty[].
 * @param  s: Channel to update.
 * @note   Call this at SYNCHRO_HZ.
 */
void synchro_update(synchro_t *s);

/**
 * @brief  Refresh duty[] from the current phase, without advancing it.
 * @param  s: Channel to compute.
 * @note   Useful before the first update, or right after a position change.
 */
void synchro_compute(synchro_t *s);

#ifdef __cplusplus
}
#endif

#endif /* SYNCHRO_H */
