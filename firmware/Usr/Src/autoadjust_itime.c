/*
 * autoadjust_itime.c
 *
 *  Created on: Jan 25, 2020
 *      Author: Bert Palm
 *
 * Algorithm as pseudo code:
 * -------------------------
 * itime:         integration time
 * MIN/MAX:       maximal/minimal (allowed) integration time (sa. globalconfig.h)
 * maxval:        the maximum value of all pixel values from the last measurement.
 * lower, upper:  the lower/upper bound of the interval for maxval. Iff
 *                `lower < maxval < upper`
 *                we accept the current integration time.
 *
 * measure
 * if maxval < lower (to low itime)
 *    itime = MAX
 *    measure
 *    if maxval < lower (still to low itime)
 *       return itime (MAXIMUM)
 *    else
 *       return binary_search_itime (1)
 *
 * if maxval > upper (saturation)
 *    itime = MIN
 *    measure
 *    if maxval > upper (still saturation)
 *       return itime (MINIMUM)
 *    else
 *       return binary_search_itime (2)
 *
 * (1): search the upper half [MAX/2 ...... MAX  ] start in the middle
 * (2): search the lower half [MIN   ...... MAX/2] start in the middle
 */

#include "lib_spectrometer.h"
#include "autoadjust_itime.h"


static int8_t get_direction(uint32_t maxval, uint32_t lower, uint32_t upper);
static uint32_t binary_search_itime( uint32_t min_itime, uint32_t max_itime, uint32_t lower, uint32_t upper);
static uint32_t get_max_pixel_value(void);


/**
 * Search last measurement data (every pixel value) for the maximum.
 */
static uint32_t get_max_pixel_value(void) {
	uint32_t maxval = 0;
	uint32_t *p = (uint32_t *) (sens1.data->wptr - MSPARAM_PIXEL);
	for (uint16_t i = 0; i < MSPARAM_PIXEL; ++i) {
		maxval = MAX(maxval, p[i]);
	}
	return maxval;
}

/**
 * Calculate the direction of the next correction.
 * Return -1 for downwards / lower
 * Return  0 if no correction needed
 * Return  1 for upwards / higher
 */
static int8_t get_direction(uint32_t maxval, uint32_t lower, uint32_t upper) {
	if (maxval > upper) {
		return DOWN;
	} else if (maxval < lower) {
		return UP;
	}
	return FOUND;
}

/**
 * Binary search, break on condition (maximum value is in defined bounds)
 * The search starts in the middle between the given max and min itimes.
 */
static uint32_t binary_search_itime( uint32_t min_itime, uint32_t max_itime, uint32_t lower, uint32_t upper) {

	uint32_t correction = (max_itime - min_itime) / 2;
	uint32_t itime = min_itime + correction;
	uint32_t maxval;
	int8_t dir;

	uint8_t maxtrys = 16;
	for (uint8_t i = 0; i < maxtrys; ++i) {

		sensor_measure(itime);
		maxval = get_max_pixel_value();
		dir = get_direction(maxval, lower, upper);

		correction /= 2;

		if (dir == UP){
			itime += correction;
		}

		if (dir == DOWN){
			itime -= correction;
		}

		// We found the usable itime or something
		// went wrong. In either case, we are good.
		if (dir == FOUND){
			break;
		}
		// Safety condition
		if (itime >= MAX_INTERGATION_TIME){
			itime = MAX_INTERGATION_TIME;
			break;
		}
		// Safety condition
		if (itime <= MIN_INTERGATION_TIME){
			itime = MIN_INTERGATION_TIME;
			break;
		}
	}
	sensor_deinit();
	return itime;
}

/**
 * Search for a good-choice integration time for the current
 * light conditions.
 *
 * Note: Worst case timing is about 16
 *
 * Side-effect: At last a measurement with the returned integration time was made.
 * So one could use the data from this measurement directly.
 */
uint32_t autoadjust_itime(uint32_t lower, uint32_t upper){
//uint32_t autoadjust_itime(void) {

	// first check the middle: (1sec-54ms)/2 ~ 500ms
	uint32_t itime = (MAX_INTERGATION_TIME - MIN_INTERGATION_TIME) / 2;

//	uint32_t lower = 33000; // ~50% (with darkcurrent 6000 and saturation 60'000)
//	uint32_t upper = 54000; // ~90%
	uint32_t maxval;
	int8_t dir;

	sensor_init();

	// try the middle value
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
			itime = binary_search_itime(MAX_INTERGATION_TIME/2, MAX_INTERGATION_TIME, lower, upper);
		}
		// else:
		// either we already have a useful itime, or we still have
		// a too low itime, but we already are at the very possible maximum

	} else if (dir == DOWN) {
		// we have a to high itime, so we try the maximum
		itime = MIN_INTERGATION_TIME;
		sensor_measure(itime);
		maxval = get_max_pixel_value();
		dir = get_direction(maxval, lower, upper);
		if (dir == UP) {
			itime = binary_search_itime(MIN_INTERGATION_TIME, MAX_INTERGATION_TIME/2, lower, upper);
		}
		// else:
		// either we found a useful itime, or we still have
		// a too low itime, but we already are at the very possible minimum
	}
	sensor_deinit();
	return itime;
}
