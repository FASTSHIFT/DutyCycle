/**
 * @file   synchro.c
 * @brief  Three-phase sine generator, pure integer implementation.
 * @note   See synchro.h for the contract.
 */

#include "synchro.h"

/**
 * @brief  Duty mid point.
 */
#define RES_CENTER (SYNCHRO_RES / 2)

/**
 * @brief  Maximum duty swing away from the mid point.
 * @note   The four counts of headroom keep the extremes off the rails, which
 *         avoids losing a PWM edge in either direction.
 */
#define RES_SWING (RES_CENTER - 4)

/**
 * @brief  Numerator of 2 / sqrt(3) = 1.15470, the gain that common-mode
 *         injection buys back.
 * @note   Injecting the mean of the three sine values pulls the peak down by
 *         sqrt(3)/2, so the raw sine coefficient is scaled back up by this
 *         factor. That is what makes SYNCHRO_OVERMOD = 100 fill the linear
 *         range exactly, without clipping.
 */
#define SVPWM_NUM 11547

/**
 * @brief  Denominator of 2 / sqrt(3).
 */
#define SVPWM_DEN 10000

/**
 * @brief  Duty swing at 100% amplitude, resolved at compile time.
 * @note   Widened to 64 bit because the product reaches 8.3e9 at the maximum
 *         useful resolution, which would overflow int32 and silently corrupt
 *         the amplitude. It is a compile-time constant, so this costs nothing
 *         at run time.
 */
#define GAIN_FULL \
    ((int32_t)(((int64_t)RES_SWING * SYNCHRO_OVERMOD * SVPWM_NUM) / (100 * SVPWM_DEN)))

/**
 * @brief  Speed to phase step multiplier: step = dps * 2^32 / (360 * SYNCHRO_HZ).
 * @note   A Q8 multiplier keeps this out of 64-bit divide territory at run time.
 */
#define STEP_MUL                                              \
    ((int32_t)(((4294967296ULL << 8) + (180ULL * SYNCHRO_HZ)) \
        / (360ULL * SYNCHRO_HZ)))

/**
 * @brief  Shift paired with STEP_MUL.
 */
#define STEP_SHIFT 8

/**
 * @brief  Largest speed that still fits the 32-bit product in STEP_MUL,
 *         about 1309 degrees per second.
 */
#define SPEED_MAX ((int16_t)(2000000000L / STEP_MUL))

/**
 * @brief  120 electrical degrees in the 32-bit phase domain.
 */
#define PHASE_120 0x55555555u

/**
 * @brief  240 electrical degrees in the 32-bit phase domain.
 */
#define PHASE_240 0xAAAAAAABu

/**
 * @brief  One whole degree in the 32-bit phase domain, round(2^32 / 360).
 */
#define PHASE_PER_DEG ((uint32_t)((4294967296ULL + 180ULL) / 360ULL))

/**
 * @brief  Quarter sine in Q15, one entry per 90/64 degrees.
 * @note   The two extra entries repeat the peak so that interpolation and the
 *         quadrant mirror can never index past the end of the table.
 */
static const int16_t quarter[66] = {
    0,
    804,
    1608,
    2410,
    3212,
    4011,
    4808,
    5602,
    6393,
    7179,
    7962,
    8739,
    9512,
    10278,
    11039,
    11793,
    12539,
    13279,
    14010,
    14732,
    15446,
    16151,
    16846,
    17530,
    18204,
    18868,
    19519,
    20159,
    20787,
    21403,
    22005,
    22594,
    23170,
    23731,
    24279,
    24811,
    25329,
    25832,
    26319,
    26790,
    27245,
    27683,
    28105,
    28510,
    28898,
    29268,
    29621,
    29956,
    30273,
    30571,
    30852,
    31113,
    31356,
    31580,
    31785,
    31971,
    32137,
    32285,
    32412,
    32521,
    32609,
    32678,
    32728,
    32757,
    32767,
    32767,
};

/**
 * @brief  Convert a 32-bit phase to a Q15 sine value.
 * @param  phase: Electrical angle, 2^32 = 360 degrees.
 * @retval Sine in Q15, -32767..32767.
 * @note   The top two bits pick the quadrant, the next 8 bits index the
 *          quarter table and the 8 bits after that interpolate. Odd quadrants
 *          are mirrored about 90 degrees, so one quarter table serves all four
 *          and the symmetry stays exact.
 */
static int32_t sin_q15(uint32_t phase)
{
    uint8_t quadrant = (uint8_t)(phase >> 30);
    uint32_t offset = phase & 0x3FFFFFFFu;
    uint8_t index;
    int32_t lo, hi, value;

    if (quadrant & 1u) {
        offset = 0x40000000u - offset;
    }

    index = (uint8_t)(offset >> 24);

    lo = quarter[index];
    hi = quarter[index + 1];

    value = lo + (((hi - lo) * (int32_t)((offset >> 16) & 0xFFu)) >> 8);

    return (quadrant >= 2u) ? -value : value;
}

/**
 * @brief  Refresh duty[] from the current phase, without advancing it.
 * @param  s: Channel to compute.
 */
void synchro_compute(synchro_t* s)
{
    int32_t v[SYNCHRO_PHASES];
    int32_t high, low, offset;
    int32_t gain = s->gain;
    uint8_t i;

    v[0] = sin_q15(s->phase);
    v[1] = sin_q15(s->phase + PHASE_240);
    v[2] = sin_q15(s->phase + PHASE_120);

    /*
     * Min-max common-mode injection. Shifting all three phases by the mean of
     * the extremes leaves every line-to-line voltage untouched but shrinks the
     * peak-to-peak demand, so the same bus can carry a larger fundamental.
     */
    high = v[0];
    low = v[0];

    if (v[1] > high)
        high = v[1];
    if (v[1] < low)
        low = v[1];
    if (v[2] > high)
        high = v[2];
    if (v[2] < low)
        low = v[2];

    offset = (high + low) >> 1;

    for (i = 0; i < SYNCHRO_PHASES; i++) {
        int32_t duty = RES_CENTER + (((v[i] - offset) * gain) >> 15);

        if (duty < 0) {
            duty = 0;
        } else if (duty > SYNCHRO_RES - 1) {
            duty = SYNCHRO_RES - 1;
        }

        s->duty[i] = (uint16_t)duty;
    }
}

/**
 * @brief  Advance the phase by one step and refresh duty[].
 * @param  s: Channel to update.
 */
void synchro_update(synchro_t* s)
{
    s->phase += (uint32_t)s->step;
    synchro_compute(s);
}

/**
 * @brief  Reset the object to angle 0, stopped, at the given amplitude.
 * @param  s: Channel to initialize.
 * @param  amp_pct: Output amplitude in percent, 0..100.
 */
void synchro_init(synchro_t* s, uint8_t amp_pct)
{
    s->phase = 0;
    s->step = 0;

    synchro_set_amplitude(s, amp_pct);
    synchro_compute(s);
}

/**
 * @brief  Set the electrical speed.
 * @param  s: Channel to configure.
 * @param  dps: Degrees per second; negative reverses the direction.
 */
void synchro_set_speed(synchro_t* s, int16_t dps)
{
    if (dps > SPEED_MAX) {
        dps = SPEED_MAX;
    } else if (dps < -SPEED_MAX) {
        dps = (int16_t)-SPEED_MAX;
    }

    s->step = ((int32_t)dps * STEP_MUL) >> STEP_SHIFT;
}

/**
 * @brief  Set the raw phase increment applied by each synchro_update().
 * @param  s: Channel to configure.
 * @param  phase_inc: Phase increment; 2^32 means one revolution per update.
 */
void synchro_set_step(synchro_t* s, int32_t phase_inc)
{
    s->step = phase_inc;
}

/**
 * @brief  Set the output amplitude.
 * @param  s: Channel to configure.
 * @param  pct: Amplitude in percent, 0..100; values above 100 are clamped.
 */
void synchro_set_amplitude(synchro_t* s, uint8_t pct)
{
    if (pct > 100) {
        pct = 100;
    }

    s->gain = (int16_t)(GAIN_FULL * pct / 100);
}

/**
 * @brief  Jump to an absolute phase.
 * @param  s: Channel to configure.
 * @param  phase: Electrical angle, 2^32 = 360 degrees.
 */
void synchro_set_phase(synchro_t* s, uint32_t phase)
{
    s->phase = phase;
}

/**
 * @brief  Jump to an absolute electrical angle in whole degrees.
 * @param  s: Channel to configure.
 * @param  deg: Angle in degrees, 0..359; larger values wrap.
 */
void synchro_set_angle(synchro_t* s, uint16_t deg)
{
    s->phase = (deg % 360u) * PHASE_PER_DEG;
}
