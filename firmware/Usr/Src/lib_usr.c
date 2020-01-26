/*
 * lib_user.c
 *
 *  Created on: Jan 25, 2020
 *      Author: rg
 */

#include "lib_usr.h"
#include "global_config.h"
#include "lib_spectrometer.h"

/**
 * measure
 * if maxval < lower (to low itime)
 *    itime = MAX
 *    measure
 *    if maxval < lower (still to low itime)
 *       return itime
 *    else
 *       return binary_search
 *
 * if maxval > upper (saturation)
 *    itime = MIN
 *    measure
 *    if maxval > upper (still saturation)
 *       return itime
 *    else
 *       return binary_search
 */

static uint32_t autoadjust_itime(void);

static uint32_t get_max_pixel_value(void) {
	uint32_t maxval = 0;
	uint32_t *p = (uint32_t *) (sens1.data->wptr - MSPARAM_PIXEL);
	for (uint16_t i = 0; i < MSPARAM_PIXEL; ++i) {
		maxval = MAX(maxval, p[i]);
	}
	return maxval;
}

int8_t get_direction(uint32_t maxval, uint32_t lower, uint32_t upper) {
	if (maxval > upper) {
		return DOWN;
	} else if (maxval < lower) {
		return UP;
	}
	return FOUND;
}

static uint32_t binary_search_itime(
		uint32_t start,
		uint32_t low_itime,
		uint32_t high_itime,
		uint32_t low_value,
		uint32_t high_value) {

	uint32_t itime = start;
	uint32_t correction = (high_itime - low_itime) / 2;
	uint32_t maxval;
	int8_t dir = 0;

	uint8_t maxtrys = 16;
	for (uint8_t i = 0; i < maxtrys; ++i) {

		sensor_measure(itime);
		maxval = get_max_pixel_value();
		dir = get_direction(maxval, low_value, high_value);
		correction /= 2;

		if (dir == UP){
			itime += correction;
		}

		if (dir == DOWN){
			itime -= correction;
		}

		// we found the usable itime or something
		// went wrong. In either case, we are good.
		if (dir == FOUND){
			break;
		}
		if (itime >= MAX_INTERGATION_TIME){
			itime = MAX_INTERGATION_TIME;
			break;
		}
		if (itime <= MAX_INTERGATION_TIME){
			itime = MAX_INTERGATION_TIME;
			break;
		}
	}
	sensor_deinit();
	return itime;
}

uint32_t get_next_itime(uint8_t shift, uint32_t start, uint32_t itime, uint32_t lower, uint32_t upper) {
	uint32_t diff = start >> shift;
	uint32_t maxval = 0;
	maxval = get_max_pixel_value();
	if (maxval > upper) {
		itime -= diff;
	} else if (maxval < lower) {
		itime += diff;
	}
	return itime;
}

//uint32_t autoadjust_itime(uint32_t dark, uint32_t saturation, uint32_t lower, uint32_t upper){
uint32_t autoadjust_itime(void) {
	// curr = (1sec-54ms)/2
//	uint32_t itime = (MAX_INTERGATION_TIME - MIN_INTERGATION_TIME) >> 1;
	uint32_t itime = (MAX_INTERGATION_TIME) >> 1;
	uint32_t saturation = 60000;
	uint32_t dark = 6000;
	uint32_t effectiv = saturation - dark;
	uint32_t lower = dark + (uint32_t) (0.50 * effectiv);
	uint32_t upper = dark + (uint32_t) (0.90 * effectiv);
	uint32_t maxval;
	int8_t dir;

	sensor_init();
	sensor_measure(itime);
	maxval = get_max_pixel_value();
	dir = get_direction(maxval, lower, upper);

	if (dir == UP) {
		// we have a to low itime, so we try the maximum
		itime = MAX_INTERGATION_TIME;
		sensor_measure(itime);
		maxval = get_max_pixel_value();
		dir = get_direction(maxval, lower, upper);
		if (dir == DOWN) {
			itime = binary_search_itime(MAX_INTERGATION_TIME/2, MAX_INTERGATION_TIME/2, MAX_INTERGATION_TIME, lower, upper);
		}
		// else:
		// either we already have a useful itime, or we still have
		// a to low itime, but we already have the very possible maximum

	} else if (dir == DOWN) {
		// we have a to high itime, so we try the maximum
		itime = MIN_INTERGATION_TIME;
		sensor_measure(itime);
		maxval = get_max_pixel_value();
		dir = get_direction(maxval, lower, upper);
		if (dir == UP) {
			itime = binary_search_itime(MIN_INTERGATION_TIME, MIN_INTERGATION_TIME, MAX_INTERGATION_TIME/2, lower, upper);
		}
		// else:
		// either we found a useful itime, or we still have
		// a to high itime, but we already have the very possible minimum
	}
	sensor_deinit();
	return itime;
}
