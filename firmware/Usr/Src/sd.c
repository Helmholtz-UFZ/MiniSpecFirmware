/*
 * sd.c
 *
 *  Created on: Jul 5, 2019
 *      Author: palmb_ubu
 */

#include <lib_sd.h>
#include <lib_spectrometer.h>
#include "sd.h"
#include "fatfs.h"
#include "string.h"
#include "stdio.h"
#include "cmd_parser.h"
#include "main_usr.h"
#include "stdio_usr.h"

static filename_t fname = { .buf = { 0 }, .postfix = 0 };
FIL *f = &SDFile;

void inform_SD_reset(void) {
#if HAS_SD
	uint8_t res = 0;
	/* Inform the File that an reset occurred */
	res = sd_mount();
	if (!res) {
		res = sd_find_right_filename(fname.postfix, &fname.postfix, fname.buf, FNAME_BUF_SZ);
		if (!res) {
			res = sd_open_file_neworappend(f, fname.buf);
			if (!res) {
				res = f_printf(f, "\nThe sensor was reset/powered-down.\n");
				res = sd_close(f);
			}
		}
		res = sd_umount();
	}
#else
	return;
#endif
}

void inform_SD_rtc(char *oldtimestamp_str) {
#if HAS_SD
	uint16_t sz = 32;
	char buf[sz];
	uint8_t res = 0;
	res = sd_mount();
	if (!res) {
		res = sd_find_right_filename(fname.postfix, &fname.postfix, fname.buf, FNAME_BUF_SZ);
		if (!res) {
			res = sd_open_file_neworappend(f, fname.buf);
			if (!res) {
				f_printf(f, "The RTC was set. Old time: %S\n", oldtimestamp_str);
				rtc_get_now_str(buf, sz);
				f_printf(f, "                 New time: %S\n", buf);
				sd_close(f);
			}
		}
		sd_umount();
	}
#else
	return;
#endif
}

uint8_t write_config_to_SD(runtime_config_t *rc) {
#if HAS_SD
	uint8_t res = 0;
	res = sd_mount();
	if (!res) {
		res = sd_open(f, SD_CONFIGFILE_NAME, FA_WRITE | FA_CREATE_ALWAYS);
		if (!res) {
			f_printf(f, "%U\n", RCCONF_MAX_ITIMES);
			for (int i = 0; i < RCCONF_MAX_ITIMES; ++i) {
				f_printf(f, "%LU\n", rc->itime[i]);
			}
			f_printf(f, "%U\n", rc->iterations);

			/* We store the ival like the user command format:
			 * 'mode,ival,start,end'
			 * e.g.: '1,00:00:20,04:30:00,22:15:00*/
			f_printf(f, "%U,", rc->mode);
			f_printf(f, "%02U:%02U:%02U,", rc->ival.Hours, rc->ival.Minutes, rc->ival.Seconds);
			f_printf(f, "%02U:%02U:%02U,", rc->start.Hours, rc->start.Minutes, rc->start.Seconds);
			f_printf(f, "%02U:%02U:%02U\n", rc->end.Hours, rc->end.Minutes, rc->end.Seconds);
			res = sd_close(f);
		}
		sd_umount();
	}
	return res;
#else
	return NO_SD;
#endif
}

uint8_t read_config_from_SD(runtime_config_t *rc) {
	// todo save whole rc to SD (debugprints, ...)
#if HAS_SD
# if RCCONF_MAX_ITIMES > 32
# error "Attention buffer gets big.. Improve implementation :)"
# endif
	/* max value of itime = 10000\n -> 6 BYTES * RCCONF_MAX_ITIMES
	 * start end ival timestamp     -> 3 * 20 BYTES
	 * three other number good will aprox. -> 20 BYTES*/
	uint16_t sz = RCCONF_MAX_ITIMES * 6 + 3 * 20 + 20;
	uint8_t buf[sz];
	uint8_t res;
	uint32_t nr, rcconf_max_itimes;
	UINT bytesread;
	char *token, *rest;
	bool fail;

	memset(buf, 0, sz * sizeof(uint8_t));
	rest = (char*) buf;
	nr = 0;
	fail = false;

	res = sd_mount();
	if (!res) {
		res = sd_open(f, SD_CONFIGFILE_NAME, FA_READ);
		if (!res) {
			f_read(f, buf, sizeof(buf), &bytesread);
			/* == parsing: == */
			/* Read RCCONF_MAX_ITIMES from sd*/
			token = strtok_r(rest, "\n", &rest);
			fail = (token == NULL);
			res = sscanf(token, "%lu", &nr);
			if (!fail && res > 0 && nr > 0) {
				rcconf_max_itimes = nr > RCCONF_MAX_ITIMES ? RCCONF_MAX_ITIMES : nr;
				/*read itimes[i] from sd*/
				for (uint i = 0; i < rcconf_max_itimes; ++i) {
					token = strtok_r(rest, "\n", &rest);
					res = sscanf(token, "%lu", &nr);
					if (token == NULL || res <= 0) {
						fail = true;
						break;
					}
					rc->itime[i] = nr;
				}
			}
			if (!fail) {
				/* Read iterations aka. N from sd*/
				token = strtok_r(rest, "\n", &rest);
				res = sscanf(token, "%lu", &nr);
				if (token != NULL && res > 0 && nr > 0) {
					rc->iterations = nr;
					/* Read 'mode,ival,start,end' as one string from sd.
					 * If parse_ival() fails, no times are set. */
					token = rest;
					fail = parse_ival(token, rc);
					res = 0;
				}
			}
			sd_close(f);
		}
		sd_umount();
	}
	if (res){
		return res;
	}
	return fail;
#else
	return NO_SD;
#endif
}

/** Store the measurment data to SD card.
 * Requires a mounted SD card. */
uint8_t measurement_to_SD(char *timestamp_str) {
#if HAS_SD
	int8_t res = 0;
	res = sd_find_right_filename(fname.postfix, &fname.postfix, fname.buf, FNAME_BUF_SZ);
	if (!res) {
		res = sd_open_file_neworappend(f, fname.buf);
		if (!res) {
			/* Write metadata (timestamp, errorcode, intergartion time) */
			f_printf(f, "%S, %U, %LU, [,", timestamp_str, sens1.errc, sens1.itime);
			/* Write data */
			if (!sens1.errc) {
				/* Lopp through measurement results and store to file */
				uint32_t *p = (uint32_t *) (sens1.data->wptr - MSPARAM_PIXEL);
				for (uint16_t i = 0; i < MSPARAM_PIXEL; ++i) {
					f_printf(f, "%U,", *(p++));
				}
			}
			f_printf(f, "]\n");
			res = sd_close(f);

			/* Print what we wrote to sd.*/
			debug("SD: wrote to File: %s, data:\n", fname.buf);
			if (rc.use_debugprints) {
				/* Use printf() instead of debug() to prevent 'dbg:' string before every value. */
				printf("%s, %u, %lu, [,", timestamp_str, sens1.errc, sens1.itime);
				uint32_t *p = (uint32_t *) (sens1.data->wptr - MSPARAM_PIXEL);
				for (uint16_t i = 0; i < MSPARAM_PIXEL; ++i) {
					printf("%u,", (uint) *(p++));
				}
				printf("]\n");
			}
		}
	}
	return res;
#else
	return NO_SD;
#endif
}

