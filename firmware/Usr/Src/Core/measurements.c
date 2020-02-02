/*
 * measurements.c
 *
 *  Created on: Feb 2, 2020
 *      Author: rg
 */

#include "measurements.h"
#include "datetime.h"
#include "sysrc.h"

static uint32_t _get_full_itime(uint8_t idx);
static uint32_t _get_full_itime(uint8_t idx){
	if (rc.itime[idx] < 0) {
		return autoadjust_itime(rc.aa_lower, rc.aa_upper);
	}
	return rc.itime[idx];
}


void single_measurement(void) {
	uint32_t itime = _get_full_itime(rc.itime_index);
	if (itime == 0) {
		errreply("intergration time not set\n");
		return;
	}
	ok();
	sensor_init();
	sensor_measure(itime);
	sensor_deinit();
}



void multimeasure(bool to_sd) {
	int8_t res = 0;
	uint32_t itime = 0;
	char ts_buff[TS_BUFF_SZ] = {0, };

	for (uint8_t i = 0; i < RCCONF_MAX_ITIMES; ++i) {

		itime = _get_full_itime(i);

		/* current itime is disabled */
		if (itime == 0) {
			continue;
		}

		debug(1,"(mm): itime[%u]=%ld\n", i, itime);

		/* Measure N times */
		for (int n = 0; n < rc.iterations; ++n) {

			debug(1,"(mm): N: %u/%u\n", n, rc.iterations - 1);
			/* Generate timestamp */
			rtc_get_now_str(ts_buff, TS_BUFF_SZ);

			/* Make a measurement */
			sensor_init();
			sensor_measure(itime);

			if (to_sd && HAS_SD) {
				/* Write measurement to SD */
				/* TODO One time mount and open per wakeup... */
				res = sd_mount();
				if (!res) {
					/* Store the measurement on SD */
					res = measurement_to_SD(ts_buff);
					sd_umount();
				}
			}
			if (sens1.errc) {
				sensor_deinit();
			}

			if (rc.debuglevel == 3) {
				/* Use printf() instead of debug() to prevent 'dbg:' string before every value. */
				debug(3, "(mm): %s, %u, %lu, [,", ts_buff, sens1.errc, sens1.last_itime);
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

