/*  Easing animation library (fixed-point Q1.15 version)
    Based on original work by uYanki (https://github.com/uYanki)
    Fixed-point conversion Copyright (C) 2026 _VIFEXTech

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    Original reference:
    https://github.com/uYanki/board-stm32f103rc-berial/blob/main/
    7.Example/HAL/19.GUI/03%20u8g2/02%20menu/Lib/easing/easing.h
 */
#ifndef EASING_H
#define EASING_H

/*-------------------------------------------------------------*
 *		Includes and dependencies			*
 *-------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------*
 *		Fixed-point configuration			*
 *-------------------------------------------------------------*/

/**
 * Internal progress uses Q1.15 format:
 *   - int16_t range [0, 32768] maps to [0.0, 1.0]
 *   - Precision: 1/32768 ~ 0.00003
 *   - Multiply: (a * b) >> 15, stays within int32_t
 */
#define EASING_FRAC_BITS		15
#define EASING_FRAC_ONE			(1 << EASING_FRAC_BITS)
#define EASING_FRAC_HALF		(EASING_FRAC_ONE >> 1)
#define EASING_FRAC_MUL(a, b)	((int32_t)(a) * (int32_t)(b) >> EASING_FRAC_BITS)

/*-------------------------------------------------------------*
 *		Typedefs enums & structs			*
 *-------------------------------------------------------------*/

/**
 * Position type: direct integer value (e.g. motor position -1000 ~ +1000)
 */
typedef int32_t easing_pos_t;

/**
 * Fractional type: Q1.15 fixed-point, range [0, EASING_FRAC_ONE]
 */
typedef int16_t easing_frac_t;

/**
 * Easing curve calculation function pointer.
 * Input: t in Q1.15 [0, 32768]
 * Output: curve(t) in Q1.15 [0, 32768]
 */
typedef easing_frac_t (*easing_calc_t)(easing_frac_t t);

/**
 * Tick callback function pointer, returns current time in milliseconds
 */
typedef uint32_t (*easing_tick_cb_t)(void);

/**
 * Easing mode flags
 */
typedef enum {
	EASING_MODE_BITCNT = 4,
	EASING_MODE_MASK = (1 << EASING_MODE_BITCNT) - 1,

	EASING_TIMES_SINGLE = 0 << 0,		/* Play once (default) */
	EASING_TIMES_MANYTIMES = 1 << 0,	/* Play N times */
	EASING_TIMES_INFINITE = 1 << 1,	/* Loop forever */

	EASING_TIMES_SET = EASING_MODE_BITCNT,

	EASING_DIR_FORWARD = 0 << 0,		/* Forward (default) */
	EASING_DIR_REVERSE = 1 << 2,		/* Reverse */
	EASING_DIR_BACKANDFORTH = 1 << 3,	/* Ping-pong */
} easing_mode_t;

#define EASING_MODE_DEFAULT		((easing_mode_t)(EASING_TIMES_SINGLE | EASING_DIR_FORWARD))
#define EASING_MODE_NTIMES(n)	(EASING_TIMES_MANYTIMES | ((n) << EASING_TIMES_SET))
#define EASING_INTERVAL_NONE	0

/**
 * Easing state structure
 */
typedef struct easing {
	easing_mode_t mode;			/* Operating mode flags */
	easing_calc_t calc;			/* Curve function pointer */

	/* Position (integer) */
	easing_pos_t start;			/* Start position */
	easing_pos_t stop;			/* Stop position */
	easing_pos_t offset;		/* Position offset */
	easing_pos_t delta;			/* stop - start */
	easing_pos_t current;		/* Current position */

	/* Animation progress */
	uint16_t frame_count;		/* Total frames [2, n] */
	uint16_t frame_index;		/* Current frame [0, frame_count] */
	easing_frac_t progress;		/* Current progress Q1.15 */

	int16_t times;				/* Remaining play count (-1 = infinite) */
	bool reverse;				/* true: playing in reverse */

	uint32_t tick_ms;			/* Timestamp for interval control */
	uint16_t interval;			/* Minimum ms per frame (0 = no limit) */
} easing_t;

/*-------------------------------------------------------------*
 *		Curve functions (pure integer)		*
 *-------------------------------------------------------------*/

/** @brief Linear: t */
easing_frac_t easing_calc_linear(easing_frac_t t);

/** @brief Quadratic: t^2 */
easing_frac_t easing_calc_in_quad(easing_frac_t t);
easing_frac_t easing_calc_out_quad(easing_frac_t t);
easing_frac_t easing_calc_in_out_quad(easing_frac_t t);

/** @brief Cubic: t^3 */
easing_frac_t easing_calc_in_cubic(easing_frac_t t);
easing_frac_t easing_calc_out_cubic(easing_frac_t t);
easing_frac_t easing_calc_in_out_cubic(easing_frac_t t);

/** @brief Quartic: t^4 */
easing_frac_t easing_calc_in_quart(easing_frac_t t);
easing_frac_t easing_calc_out_quart(easing_frac_t t);
easing_frac_t easing_calc_in_out_quart(easing_frac_t t);

/** @brief Quintic: t^5 */
easing_frac_t easing_calc_in_quint(easing_frac_t t);
easing_frac_t easing_calc_out_quint(easing_frac_t t);
easing_frac_t easing_calc_in_out_quint(easing_frac_t t);

/** @brief Back: 3t^3 - 2t^2 (overshoot) */
easing_frac_t easing_calc_in_back(easing_frac_t t);
easing_frac_t easing_calc_out_back(easing_frac_t t);
easing_frac_t easing_calc_in_out_back(easing_frac_t t);

/*-------------------------------------------------------------*
 *		Function prototypes				*
 *-------------------------------------------------------------*/

/**
 * @brief Set the tick callback for time-based interval control
 *
 * @param tick_cb Function returning current time in milliseconds
 */
void easing_set_tick_callback(easing_tick_cb_t tick_cb);

/**
 * @brief Initialize an easing instance
 *
 * @param e Pointer to easing state structure
 * @param mode Operating mode (direction, repeat count)
 * @param calc Curve function (NULL defaults to linear)
 * @param offset Position offset added to output
 * @param frame_count Total animation frames (minimum 2)
 * @param interval Minimum milliseconds between frames (0 = no limit)
 */
void easing_init(
	easing_t* e,
	easing_mode_t mode,
	easing_calc_t calc,
	easing_pos_t offset,
	uint16_t frame_count,
	uint16_t interval);

/**
 * @brief Start animation with absolute start/stop positions
 *
 * @param e Pointer to easing state structure
 * @param start Start position (integer)
 * @param stop Target position (integer)
 */
void easing_start_absolute(easing_t* e, easing_pos_t start, easing_pos_t stop);

/**
 * @brief Start animation with relative distance from current stop
 *
 * @param e Pointer to easing state structure
 * @param distance Distance to travel from current position
 */
void easing_start_relative(easing_t* e, easing_pos_t distance);

/**
 * @brief Advance animation by one frame
 *
 * Call this function periodically to update the easing position.
 * Respects interval timing if tick callback is set.
 *
 * @param e Pointer to easing state structure
 */
void easing_update(easing_t* e);

/**
 * @brief Check if animation has completed
 *
 * @param e Pointer to easing state structure
 * @return true if animation is finished, false if still running
 */
bool easing_isok(easing_t* e);

/**
 * @brief Force-stop the animation at a given position
 *
 * @param e Pointer to easing state structure
 * @param current Position to set as current
 */
void easing_stop(easing_t* e, easing_pos_t current);

/**
 * @brief Get current position (with offset applied)
 *
 * @param e Pointer to easing state structure
 * @return Current position + offset
 */
easing_pos_t easing_curpos(easing_t* e);

#ifdef __cplusplus
}
#endif

#endif
/* End of Header file */
