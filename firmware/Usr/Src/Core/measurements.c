/*
 * measurements.c
 *
 *  Created on: Feb 2, 2020
 *      Author: Bert Palm
 */

#include "measurements.h"
#include "sensor.h"
#include "autoadjust_itime.h"
#include "datetime.h"
#include "sysrc.h"
#include "logging.h"
#include "sd.h"

#include <stdio.h>

static uint32_t _get_full_itime(uint8_t idx);


void measure(uint32_t itime){

	if (sens1.status != SENS_INITIALIZED){
		sensor_deinit();
		sensor_init();
	}

	sensor_measure(itime);
}

void multimeasure(bool to_sd) {
	int8_t res = 0;
	uint32_t itime = 0;
	rtc_timestamp_t now;

	for (uint8_t i = 0; i < RCCONF_MAX_ITIMES; ++i) {

		itime = _get_full_itime(i);

		/* current itime is disabled */
		if (itime == 0) {
			continue;
		}

		debug(1,"(mm): itime[%u]=%ld\n", i, itime);

		/* Measure N times */
		for (int n = 0; n < rc.iterations; ++n) {

			debug(1,"(mm): N: %u/%u\n", n+1, rc.iterations);

			/* Generate timestamp */
			now = rtc_get_now();

			/* Make a measurement */
			measure(itime);

			/* Store the measurement with a timestamp to SD */
			if (to_sd) {
				res = sd_mount();
				if (!res) {
					res = sd_write_measurement(now);
					sd_umount();
				}
			}

			if (rc.debuglevel == 3) {
				/* Use printf() instead of debug() to prevent 'dbg:' string before every value. */
				debug(3, "(mm): ");
				printf(TS_TO_PRINTCALL(now));
				printf(", %u, %lu, [,", sens1.errc, sens1.last_itime);
				uint32_t *p = (uint32_t *) (sens1.data->wptr - MSPARAM_PIXEL);
				for (uint16_t i = 0; i < MSPARAM_PIXEL; ++i) {
					printf("%u,", (uint) *(p++));
				}
				printf("]\n");
			}
		}
	}
	sensor_deinit();
}

static uint32_t _get_full_itime(uint8_t idx){
	if (rc.itime[idx] < 0) {
		return autoadjust_itime(rc.aa_lower, rc.aa_upper);
	}
	return rc.itime[idx];
}
