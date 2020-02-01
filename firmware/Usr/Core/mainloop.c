/*
 * main_usr.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include <globalconfig.h>
#include <sensor.h>
#include <logging.h>
#include <mainloop.h>
#include <pprint.h>
#include <rtc_dt.h>
#include <rxtx.h>
#include <sdfs.h>
#include "power.h"
#include "rtc.h"
#include "cmd_parser.h"
#include "string.h"
#include "fatfs.h"
#include "alarm.h"
#include "sd.h"
#include <stdio.h>
#include <timer.h>
#include "autoadjust_itime.h"

static void multimeasure(bool to_sd);
static void periodic_alarm_handler(void);
static void extcmd_handler(void);
static void dbg_test(void);

// helper
static uint32_t _get_full_itime(uint8_t idx);
static void _single_measurement(void);
static void _set_rtc_time(void);
static void _print_sd_config(void);
static void _sleep(void);


runtime_config_t rc = {
		.iterations = 1,
		.itime = {DEFAULT_INTEGRATION_TIME, 0,},
		.itime_index = 0,
		.aa_lower = 33000,
		.aa_upper = 54000,
		.mode = START_MODE,
		.start = {.Hours = 99, .Minutes = 99, .Seconds = 99},
		.end = {.Hours = 99, .Minutes = 99, .Seconds = 99},
		.ival = {.Hours = 99, .Minutes = 99, .Seconds = 99},
		.next_alarm = {.Hours = 99, .Minutes = 99, .Seconds = 99},
		.debuglevel = DEBUG_DEFAULT_LVL,
		.format = DATA_FORMAT_ASCII,
		.sleeping = false,
		.trigger = false,
};


void usr_hw_init(void) {
	/* sa. Errata 2.1.3. or Arm ID number 838869 */
	uint32_t *ACTLR = (uint32_t *) 0xE000E008;
	*ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk;

	sensor_deinit();
	rxtx_init();
	rxtx_restart_listening();
}


void run_init(void) {
	power_switch_EN(ON);
	usr_hw_init();

	// inform user
	reply("\nstart\n");
	reply("firmware: %s\n", __FIRMWARE_VERSION);

	// read/write info from/to sd
	inform_SD_reset();
	read_config_from_SD(&rc);

	init_mode(&rc);
}


static void _sleep(void){
	/* Uart IR is enabled only during (light) sleep phases.
	 * If a uart IR occur during the IR is disabled, it will
	 * come up, immediately after enabling it agian.
	 * (see next line) */
	__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM);
	if (!rxtx.wakeup && !rtc.alarmA_wakeup) {
		cpu_enter_LPM();
	}
	__HAL_UART_DISABLE_IT(&hrxtx, UART_IT_CM);
}


void run(void) {

	while(true) {

		_sleep();

		if (hrxtx.ErrorCode) {
			/* See stm32l4xx_hal_uart.h for ErrorCodes.
			 * Handle Uart Errors immediately because every further
			 * send (e.g. printf) would overwrite the Error Code.*/
			rxtx_restart_listening();
		}

		debug(3,"(main): rxtx,alarm,trigger: %i %i %i\n", rxtx.wakeup, rtc.alarmA_wakeup, rc.trigger);

		if (rc.trigger) {
			rc.trigger = false;
			if (rc.mode == MODE_TRIGGERED){
				multimeasure(true);
			}
		}

		if (rtc.alarmA_wakeup) {
			rtc.alarmA_wakeup = false;
			if(rc.mode == MODE_ENDLESS || rc.mode == MODE_STARTEND){
				periodic_alarm_handler();
			}
		}

		if (rxtx.wakeup) {
			parse_extcmd(rxtx_rxbuffer.base, rxtx_rxbuffer.size);
			rxtx.wakeup = false;
			rxtx.cmd_bytes = 0;
			rxtx_restart_listening();
			extcmd_handler();
		}
	}
}


static void extcmd_handler(void) {
	int32_t nr = 0, nr1 = 0;
	char *str = NULL;

	switch (extcmd.cmd) {

	case USR_CMD_SINGLE_MEASURE_START:
		_single_measurement();
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
		dbg_test();
		break;

	case USR_CMD_SET_SENSOR:
	case USR_CMD_UNKNOWN:
	default:
		reply("???\n");
		break;
	}
}


static void periodic_alarm_handler(void) {
	debug(1,"(alarm): Periodic alarm \n");
	RTC_TimeTypeDef new;
	rtc_timestamp_t ts = {0,};
	ts = rtc_get_now();
	debug(2,"(alarm): now: 20%02i-%02i-%02iT%02i:%02i:%02i\n", ts.date.Year, ts.date.Month, ts.date.Date, ts.time.Hours,
			ts.time.Minutes, ts.time.Seconds);

	/* set new alarm */
	new = rtc_time_add(&rc.next_alarm, &rc.ival);
	if (rc.mode == MODE_ENDLESS) {
		rtc_set_alarmA(&new);

	} else { /* rc.mode == IVAL_STARTEND */
		/* if new <= end */
		if (rtc_time_leq(&new, &rc.end)) {
			rtc_set_alarmA(&new);
		} else {
			rtc_set_alarmA(&rc.start);
		}
	}
	rc.next_alarm = rtc_get_alermAtime();

	/* Do the measurement */
	multimeasure(true);

	debug(2,"(alarm): next: %02i:%02i:%02i\n", rc.next_alarm.Hours, rc.next_alarm.Minutes, rc.next_alarm.Seconds);
}


static void multimeasure(bool to_sd) {
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


/* This function is used to test functions
 * or functionality under development.
 * Especially if the code is hard to reach
 * in the normal program flow. E.g. If the
 * code is executed by timer. */
static void dbg_test(void) {
	return;
}


static uint32_t _get_full_itime(uint8_t idx){
	if (rc.itime[idx] < 0) {
		return autoadjust_itime(rc.aa_lower, rc.aa_upper);
	}
	return rc.itime[idx];
}


static void _single_measurement(void) {
	uint32_t itime = _get_full_itime(rc.itime_index);
	if (itime == 0) {
		errreply("intergration time not set\n");
		return;
	}
	ok();
	sensor_init();
	sensor_measure(itime);
	sensor_deinit();
	send_data();
}


static void _set_rtc_time(void){
	char *str = NULL;
	rtc_timestamp_t ts = {0,};
	char ts_buff[TS_BUFF_SZ] = {0, };

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
	rtc_get_now_str(ts_buff, TS_BUFF_SZ);
	HAL_RTC_SetTime(&hrtc, &ts.time, RTC_FORMAT_BIN);
	HAL_RTC_SetDate(&hrtc, &ts.date, RTC_FORMAT_BIN);
	inform_SD_rtc(ts_buff);
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
