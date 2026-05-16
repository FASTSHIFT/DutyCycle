/*  Easing animation library (fixed-point Q1.15 version)
    Based on original work by uYanki (https://github.com/uYanki)
    Fixed-point conversion Copyright (C) 2026 _VIFEXTech

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.
 */

#include "easing.h"
#include <string.h>

/*-------------------------------------------------------------*
 *		Private variables				*
 *-------------------------------------------------------------*/

static easing_tick_cb_t tick_callback = 0;

#define get_tick_ms() tick_callback()

/*-------------------------------------------------------------*
 *		Curve implementations (Q1.15)		*
 *-------------------------------------------------------------*/

easing_frac_t easing_calc_linear(easing_frac_t t)
{
	return t;
}

easing_frac_t easing_calc_in_quad(easing_frac_t t)
{
	return EASING_FRAC_MUL(t, t);
}

easing_frac_t easing_calc_out_quad(easing_frac_t t)
{
	easing_frac_t inv = EASING_FRAC_ONE - t;
	return EASING_FRAC_ONE - EASING_FRAC_MUL(inv, inv);
}

easing_frac_t easing_calc_in_out_quad(easing_frac_t t)
{
	if (t < EASING_FRAC_HALF) {
		int32_t t2 = (int32_t)t * 2;
		return (easing_frac_t)(EASING_FRAC_MUL(t2, t2) >> 1);
	} else {
		int32_t inv = ((int32_t)EASING_FRAC_ONE - t) * 2;
		return (easing_frac_t)(EASING_FRAC_ONE - (EASING_FRAC_MUL(inv, inv) >> 1));
	}
}

easing_frac_t easing_calc_in_cubic(easing_frac_t t)
{
	return EASING_FRAC_MUL(EASING_FRAC_MUL(t, t), t);
}

easing_frac_t easing_calc_out_cubic(easing_frac_t t)
{
	easing_frac_t inv = EASING_FRAC_ONE - t;
	return EASING_FRAC_ONE - EASING_FRAC_MUL(EASING_FRAC_MUL(inv, inv), inv);
}

easing_frac_t easing_calc_in_out_cubic(easing_frac_t t)
{
	if (t < EASING_FRAC_HALF) {
		int32_t t2 = (int32_t)t * 2;
		return (easing_frac_t)(EASING_FRAC_MUL(EASING_FRAC_MUL(t2, t2), t2) >> 1);
	} else {
		int32_t inv = ((int32_t)EASING_FRAC_ONE - t) * 2;
		return (easing_frac_t)(EASING_FRAC_ONE - (EASING_FRAC_MUL(EASING_FRAC_MUL(inv, inv), inv) >> 1));
	}
}

easing_frac_t easing_calc_in_quart(easing_frac_t t)
{
	easing_frac_t t2 = EASING_FRAC_MUL(t, t);
	return EASING_FRAC_MUL(t2, t2);
}

easing_frac_t easing_calc_out_quart(easing_frac_t t)
{
	easing_frac_t inv = EASING_FRAC_ONE - t;
	easing_frac_t inv2 = EASING_FRAC_MUL(inv, inv);
	return EASING_FRAC_ONE - EASING_FRAC_MUL(inv2, inv2);
}

easing_frac_t easing_calc_in_out_quart(easing_frac_t t)
{
	if (t < EASING_FRAC_HALF) {
		int32_t t2 = (int32_t)t * 2;
		easing_frac_t t2sq = EASING_FRAC_MUL(t2, t2);
		return (easing_frac_t)(EASING_FRAC_MUL(t2sq, t2sq) >> 1);
	} else {
		int32_t inv = ((int32_t)EASING_FRAC_ONE - t) * 2;
		easing_frac_t inv2 = EASING_FRAC_MUL(inv, inv);
		return (easing_frac_t)(EASING_FRAC_ONE - (EASING_FRAC_MUL(inv2, inv2) >> 1));
	}
}

easing_frac_t easing_calc_in_quint(easing_frac_t t)
{
	easing_frac_t t2 = EASING_FRAC_MUL(t, t);
	return EASING_FRAC_MUL(EASING_FRAC_MUL(t2, t2), t);
}

easing_frac_t easing_calc_out_quint(easing_frac_t t)
{
	easing_frac_t inv = EASING_FRAC_ONE - t;
	easing_frac_t inv2 = EASING_FRAC_MUL(inv, inv);
	return EASING_FRAC_ONE - EASING_FRAC_MUL(EASING_FRAC_MUL(inv2, inv2), inv);
}

easing_frac_t easing_calc_in_out_quint(easing_frac_t t)
{
	if (t < EASING_FRAC_HALF) {
		int32_t t2 = (int32_t)t * 2;
		easing_frac_t t2sq = EASING_FRAC_MUL(t2, t2);
		return (easing_frac_t)(EASING_FRAC_MUL(EASING_FRAC_MUL(t2sq, t2sq), t2) >> 1);
	} else {
		int32_t inv = ((int32_t)EASING_FRAC_ONE - t) * 2;
		easing_frac_t inv2 = EASING_FRAC_MUL(inv, inv);
		return (easing_frac_t)(EASING_FRAC_ONE - (EASING_FRAC_MUL(EASING_FRAC_MUL(inv2, inv2), inv) >> 1));
	}
}

easing_frac_t easing_calc_in_back(easing_frac_t t)
{
	/* 3t^3 - 2t^2 */
	easing_frac_t t2 = EASING_FRAC_MUL(t, t);
	easing_frac_t t3 = EASING_FRAC_MUL(t2, t);
	return (easing_frac_t)((int32_t)t3 * 3 - (int32_t)t2 * 2);
}

easing_frac_t easing_calc_out_back(easing_frac_t t)
{
	return EASING_FRAC_ONE - easing_calc_in_back(EASING_FRAC_ONE - t);
}

easing_frac_t easing_calc_in_out_back(easing_frac_t t)
{
	if (t < EASING_FRAC_HALF) {
		easing_frac_t t2 = (easing_frac_t)((int32_t)t * 2);
		return (easing_frac_t)(easing_calc_in_back(t2) >> 1);
	} else {
		easing_frac_t inv = (easing_frac_t)(((int32_t)EASING_FRAC_ONE - t) * 2);
		return (easing_frac_t)(EASING_FRAC_ONE - (easing_calc_in_back(inv) >> 1));
	}
}

/*-------------------------------------------------------------*
 *		API implementation				*
 *-------------------------------------------------------------*/

void easing_set_tick_callback(easing_tick_cb_t tick_cb)
{
	tick_callback = tick_cb;
}

void easing_init(
	easing_t* e,
	easing_mode_t mode,
	easing_calc_t calc,
	easing_pos_t offset,
	uint16_t frame_count,
	uint16_t interval)
{
	memset(e, 0, sizeof(easing_t));
	e->mode = mode;
	e->calc = (calc == 0) ? easing_calc_linear : calc;
	e->offset = offset;
	e->frame_count = (frame_count < 2) ? 2 : frame_count;
	e->interval = interval;
	e->reverse = mode & EASING_DIR_REVERSE;
}

void easing_start_absolute(easing_t* e, easing_pos_t start, easing_pos_t stop)
{
	e->start = start;
	e->stop = stop;
	e->delta = stop - start;

	e->frame_index = 0;
	e->progress = 0;

	e->reverse = e->mode & EASING_DIR_REVERSE;

	if (e->mode & EASING_TIMES_INFINITE) {
		e->times = -1;
	} else {
		e->times = (e->mode & EASING_TIMES_MANYTIMES)
			? (e->mode >> EASING_TIMES_SET) : 1;
		if (e->mode & EASING_DIR_BACKANDFORTH)
			e->times *= 2;
	}

#ifdef get_tick_ms
	e->tick_ms = get_tick_ms();
#endif
}

void easing_start_relative(easing_t* e, easing_pos_t distance)
{
	easing_start_absolute(e, e->current, e->stop + distance);
}

void easing_update(easing_t* e)
{
	/* Check if finished */
	if (e->times == 0)
		return;

#ifdef get_tick_ms
	/* Interval timing control */
	if (e->interval > 0) {
		if (get_tick_ms() < e->tick_ms)
			return;
		e->tick_ms = get_tick_ms() + e->interval;
	}
#endif

	/* Advance frame */
	++e->frame_index;

	if (e->frame_index > e->frame_count) {
		if (e->mode & EASING_DIR_BACKANDFORTH) {
			e->reverse = !e->reverse;
			e->frame_index = 2;
		} else {
			e->frame_index = 1;
		}
	}

	if (e->frame_index == e->frame_count) {
		/* Last frame: snap to endpoint */
		e->progress = EASING_FRAC_ONE;
		e->current = e->reverse ? e->start : e->stop;

		if (!(e->mode & EASING_TIMES_INFINITE))
			if (--e->times)
				return;
	} else {
		/* Calculate progress Q1.15 */
		e->progress = (easing_frac_t)(
			(int32_t)(e->frame_index - 1) * EASING_FRAC_ONE
			/ (e->frame_count - 1));

		/* Apply curve function */
		easing_frac_t curve = e->calc(e->progress);

		/* Calculate integer position */
		if (e->reverse) {
			e->current = e->stop
				- (int32_t)e->delta * curve / EASING_FRAC_ONE;
		} else {
			e->current = e->start
				+ (int32_t)e->delta * curve / EASING_FRAC_ONE;
		}
	}
}

bool easing_isok(easing_t* e)
{
	return e->times == 0;
}

void easing_stop(easing_t* e, easing_pos_t current)
{
	e->times = 0;
	e->current = current;
}

easing_pos_t easing_curpos(easing_t* e)
{
	return e->current + e->offset;
}
