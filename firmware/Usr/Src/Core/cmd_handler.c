/*
 * cmd_handler.c
 *
 *  Created on: Feb 2, 2020
 *      Author: Bert Palm
 */


#include "cmd_handler.h"

#include "sysrc.h"
#include "pprint.h"
#include "logging.h"
#include "measurements.h"
#include "cmd_parser.h"

#include <stddef.h>
#include <stdbool.h>

static void _dbg_test(void);
static void _set_rtc_time(void);
static void _print_sd_config(void);


// todo rename cmdHandler()
void extcmd_handler(void) {
	int32_t nr = 0, nr1 = 0;
	char *str = NULL;

	switch (extcmd.cmd) {

	case USR_CMD_SINGLE_MEASURE_START:
		single_measurement();
		send_data();
		break;

	case USR_CMD_MULTI_MEASURE_START:
		ok();
		multimeasure(false);
		break;

	case USR_CMD_VERSION:
		reply("firmware: %s\n", __FIRMWARE_VERSION);
		break;

	case USR_CMD_GET_DATA:
		send_data();
		break;

	case USR_CMD_GET_ITIME:
		send_itime();
		break;

	case USR_CMD_GET_RTC_TIME:
		send_rtc_time();
		break;

	case USR_CMD_GET_CONFIG:
		print_config(&rc, "SYSTEM CONFIG");
		break;

	case USR_CMD_SET_FORMAT:
		if (argparse_nr(&nr)) { argerr(); break; }
		rc.format = (nr > 0) ? DATA_FORMAT_ASCII : DATA_FORMAT_BIN;
		ok(); break;

	case USR_CMD_SET_ITIME:
		if (argparse_nr(&nr)) { argerr(); break; }
		// itime <  0: auto measure, itime == 0: disable index, itime >  0: check min/max
		nr = nr < 0 ? -1 : nr > 0 ? MIN(MAX_INTERGATION_TIME, MAX(MIN_INTERGATION_TIME, nr)) : 0;
		rc.itime[rc.itime_index] = nr;
		ok(); break;

	case USR_CMD_SET_ITIME_INDEX:
		if (argparse_nr(&nr)) { argerr(); break; }
		if (0 <= nr && nr < RCCONF_MAX_ITIMES) {
			rc.itime_index = nr;
			ok();
		}else{
			argerr();
		}
		break;

	case USR_CMD_SET_MULTI_MEASURE_ITERATIONS:
		if (argparse_nr(&nr)) { argerr(); break; }
		if (0 < nr && nr < RCCONF_MAX_ITERATIONS) {
			rc.iterations = nr;
			ok();
		}else{
			argerr();
		}
		break;

	case USR_CMD_SET_RTC_TIME:
		_set_rtc_time();
		break;

	case USR_CMD_SET_MODE:
		if (argparse_str(&str)) { argerr(); break; }
		if (parse_mode(str, &rc)) { argerr(); break; }
		init_mode(&rc);
		ok(); break;

	case USR_CMD_SET_AUTOADJUST_PARAMS:
		if (argparse_nrs(&nr, &nr1)) { argerr(); break; }
		// constrains
		if (nr > UINT16_MAX || nr1 > UINT16_MAX || nr < 0 || nr1 < 0 || nr > nr1) { argerr(); break; }
		rc.aa_lower = nr;
		rc.aa_upper = nr1;
		// no break
	case USR_CMD_TEST_AUTOADJUST:
		nr = autoadjust_itime(rc.aa_lower, rc.aa_upper);
		reply("upper, lower:  %ld, %ld\n", rc.aa_lower, rc.aa_upper);
		reply("test aa-itime: %ld us\n", nr);
		break;

	case USR_CMD_HELP:
		reply(HELPSTR);
		break;

	case USR_CMD_STORE_SDCONFIG:
		if (write_config_to_SD(&rc)){ errreply("write to SD faild\n"); break; }
		ok();
		break;

	case USR_CMD_PRINT_SDCONFIG:
			_print_sd_config();
			break;

	case USR_CMD_READ_SDCONFIG:
		if (read_config_from_SD(&rc)){ errreply("read from SD faild\n"); break; }
		init_mode(&rc);
		ok();
		break;

	case USR_CMD_DEBUG:
		if (argparse_nr(&nr)) { argerr(); break; }
		rc.debuglevel = nr;
		debug(rc.debuglevel, "(main): level set to %u\n", rc.debuglevel);
		break;

	case USR_CMD_DBGTEST:
		_dbg_test();
		break;

	case USR_CMD_SET_SENSOR:
	case USR_CMD_UNKNOWN:
	default:
		reply("???\n");
		break;
	}
}


static void _set_rtc_time(void){
	char *str = NULL;
	rtc_timestamp_t ts = {0,};
	rtc_timestamp_t now = {0,};

	if (argparse_str(&str)) {
		argerr();
		return;
	}
	if (rtc_parsecheck_datetime(str, &ts.time, &ts.date)) {
		argerr();
		return;
	}
	/* store the current time to later inform the sd
	 * with old and new time*/
	now = rtc_get_now();
	// todo new func dt_set_timestamp()
	HAL_RTC_SetTime(&hrtc, &ts.time, RTC_FORMAT_BIN);
	HAL_RTC_SetDate(&hrtc, &ts.date, RTC_FORMAT_BIN);
//	inform_SD_rtc(&now); // fixme / testme
	ok();
}


static void _print_sd_config(void){
	runtime_config_t rc_ = {0,};
	if (read_config_from_SD(&rc_)){
		errreply("(main) read failed\n");
		return;
	}
	init_timetype(&rc_.next_alarm);
	print_config(&rc_, "SD-CONFIG");
}


/* This function is used to test functions
 * or functionality under development.
 * Especially if the code is hard to reach
 * in the normal program flow. E.g. If the
 * code is executed by timer. */
static void _dbg_test(void) {
	return;
}
