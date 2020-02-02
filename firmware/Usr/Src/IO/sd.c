/*
 * sd.c
 *
 *  Created on: Jul 5, 2019
 *      Author: Bert Palm
 */

#include <logging.h>
#include <mainloop.h>
#include <sdfs.h>
#include <sensor.h>
#include "sd.h"
#include "fatfs.h"
#include "string.h"
#include "stdio.h"
#include "cmd_parser.h"

static filename_t fname = { .buf = { 0 }, .postfix = 0 };
FIL *f = &SDFile;

// todo rename sd_...()

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

			/* 1. store the number of following itimes
			 * 2. store all the itimes */
			f_printf(f, "%U\n", RCCONF_MAX_ITIMES);
			for (int i = 0; i < RCCONF_MAX_ITIMES; ++i) {
				f_printf(f, "%LD\n", rc->itime[i]);
			}

			/*store N*/
			f_printf(f, "%U\n", rc->iterations);

			/* We store the ival like the user command format:
			 * 'mode,ival,start,end'
			 * e.g.: '1,00:00:20,04:30:00,22:15:00*/
			f_printf(f, "%U,", rc->mode);
			f_printf(f, "%02U:%02U:%02U,", rc->ival.Hours, rc->ival.Minutes, rc->ival.Seconds);
			f_printf(f, "%02U:%02U:%02U,", rc->start.Hours, rc->start.Minutes, rc->start.Seconds);
			f_printf(f, "%02U:%02U:%02U\n", rc->end.Hours, rc->end.Minutes, rc->end.Seconds);

			/* NEW with version 3.0.1:
			 * store the other rc params:
			 * Note: some might be misplaced here but we want to stay
			 * backwards compatible to older configs.*/
			f_printf(f, "%U\n",  rc->format);
			f_printf(f, "%U\n",  rc->itime_index);
			f_printf(f, "%U\n",  rc->debuglevel);
			f_printf(f, "%LU\n", (uint16_t) rc->aa_lower);
			f_printf(f, "%LU\n", (uint16_t) rc->aa_upper);
			res = sd_close(f);
		}
		sd_umount();
	}
	return res;
#else
	return NO_SD;
#endif
}

/**
 * Reads the config from the sd and fill the rc (runtime-config) by its content.
 *
 * Return 0 on success, non-zero otherwise. If the parsing fails the rc may already be changed in
 * some parts.
 *
 * Note: This also works for any config files, nevertheless it was generated under linux (LF),
 * windows (CRLF) or mac (CR). The lineend is auto-determined.
 *
 * Note: This is also backwards compatible and works with config files created for any
 * version before 3.0.1
 *
 * Note: The actual filename can be set in globalconfig.h
 */
uint8_t read_config_from_SD(runtime_config_t *rc) {
#if HAS_SD
# if RCCONF_MAX_ITIMES > 32
# error "Attention buffer gets big.. Improve implementation :)"
# endif
	/* max value of itime = 10000\n -> 6 BYTES * RCCONF_MAX_ITIMES
	 * start end ival timestamp     -> 3 * 20 BYTES
	 * three other number good will aprox. -> 20 BYTES*/
	uint16_t sz = RCCONF_MAX_ITIMES * 6 + 3 * 20 + 20;
	uint8_t buf[sz];
	int8_t res;
	uint32_t rcconf_max_itimes, sdconf_max_itimes;

	// parsing vars
	uint32_t lu32 = 0;
	uint32_t lu32_2 = 0;
	int32_t ld32 = 0;
	uint u = 0;
	int d = 0;

	UINT bytesread;
	char *token, *rest;
	char *delim = "\n";

	memset(buf, 0, sz * sizeof(uint8_t));
	rest = (char*) buf;

	res = sd_mount();
	if(res){
		goto l_fail;
	}
	res = sd_open(f, SD_CONFIGFILE_NAME, FA_READ);
	if(res){
		goto l_fail;
	}

	res = f_read(f, buf, sizeof(buf), &bytesread);
	if(res){
		goto l_close;
	}

	// determine line end
	// linux:    \n   use: \n
	// windows:  \r\n use: \n  (/r is ignored during sscanf)
	// mac:      \r   use: \r
	token = strchr(rest, '\n');
	if (token == NULL){
		delim = "\r";
		token = strchr(rest, '\r');
		if (token == NULL){
			goto l_close;
		}
	}

	/* == parsing: == */

	/* Read RCCONF_MAX_ITIMES from sd*/
	token = strtok_r(rest, delim, &rest);
	if(token == NULL){ goto l_close; }

	// parse number that indicates how many itimes are written in the config file
	res = sscanf(token, "%lu", &lu32);
	if(res <= 0 || lu32 <= 0){ goto l_close; }
	sdconf_max_itimes = lu32;
	UNUSED(sdconf_max_itimes);

	rcconf_max_itimes = lu32 > RCCONF_MAX_ITIMES ? RCCONF_MAX_ITIMES : lu32;

	/*parse itimes[i] from sd
	 * todo future: parse all even if RCCONF_MAX_ITIMES is smaller*/
	for (uint i = 0; i < rcconf_max_itimes; ++i) {
		token = strtok_r(rest, delim, &rest);
		res = sscanf(token, "%ld", &ld32);
		if (token == NULL || res <= 0) { goto l_close; }
		rc->itime[i] = ld32;
	}

	/* Read iterations aka. N from sd*/
	token = strtok_r(rest, delim, &rest);
	res = sscanf(token, "%lu", &lu32);
	if(res <= 0 || token == NULL){ goto l_close; }
	rc->iterations = lu32 > 0 ? lu32 : 1;

	/* Read 'mode,ival,start,end' as one string from sd.
	 * If parse_ival() fails, no times are set. */
	token = strtok_r(rest, delim, &rest);
	if (token == NULL){ goto l_close; }
	res = parse_mode(token, rc);
	if(res != 0){ goto l_close; }

	/* NEW with version 3.0.1:
	 * read the other rc-params:
	 * Note: some might be misplaced here but we want to stay
	 * backwards compatible to older configs.*/

	// format
	token = strtok_r(rest, delim, &rest);
	res = sscanf(token, "%u", &u);
	if (token == NULL || res <= 0){ goto l_done; }
	rc->format = u > 0;

	// itime index
	token = strtok_r(rest, delim, &rest);
	res = sscanf(token, "%u", &u);
	if (token == NULL || res <= 0){ goto l_done; }
	rc->itime_index = u < RCCONF_MAX_ITIMES ? u : 0;

	// debuglevel
	token = strtok_r(rest, delim, &rest);
	res = sscanf(token, "%d", &d);
	if (token == NULL || res <= 0){ goto l_done; }
	rc->debuglevel = d;

	// aa lower and upper
	token = strtok_r(rest, delim, &rest);
	res = sscanf(token, "%lu", &lu32);
	if (token == NULL || res <= 0){ goto l_done; }
	token = strtok_r(rest, delim, &rest);
	res = sscanf(token, "%lu", &lu32_2);
	if (token == NULL || res <= 0){ goto l_done; }
	rc->aa_lower = lu32;
	rc->aa_upper = lu32_2;

	l_done:
	sd_close(f);
	sd_umount();
	return 0;

	l_close:
	sd_close(f);
	l_fail:
	sd_umount();
	return 1;

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
			f_printf(f, "%S, %U, %LU, [,", timestamp_str, sens1.errc, sens1.last_itime);
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

			debug(1, "(sd) wrote to File: %s, data:\n", fname.buf);
		}
	}
	return res;
#else
	return NO_SD;
#endif
}

