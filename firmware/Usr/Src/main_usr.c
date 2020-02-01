/*
 * main_usr.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include <lib_rtc.h>
#include <lib_sd.h>
#include <lib_spectrometer.h>
#include <lib_timer.h>
#include <lib_uart.h>
#include "main_usr.h"
#include "power.h"
#include "rtc.h"
#include "global_config.h"
#include "cmd_parser.h"
#include "string.h"
#include "fatfs.h"
#include "alarm.h"
#include "sd.h"
#include <stdio.h>
#include "stdio_usr.h"
#include "autoadjust_itime.h"

static void send_data(void);
static void multimeasure(bool to_sd);
static void periodic_alarm_handler(void);
static void extcmd_handler(void);
static void dbg_test(void);
static void print_config_from_SD(void);

runtime_config_t rc;

static rtc_timestamp_t ts;

#define TS_BUFF_SZ	32
char ts_buff[TS_BUFF_SZ];

void init_timetype(RTC_TimeTypeDef *time) {
	time->Hours = 99;
	time->Minutes = 99;
	time->Seconds = 99;
}

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

	memset(&ts, 0, sizeof(ts));
	memset(&rc, 0, sizeof(rc));

	// init runtime config
	rc.debuglevel = DEFAULT_DEBUG_LVL;
	rc.format = DATA_FORMAT_ASCII;
	rc.iterations = 1;
	rc.itime[0] = DEFAULT_INTEGRATION_TIME;
	rc.mode = MODE_OFF;
	rc.trigger = false;
	rc.aa_lower = 33000; // ~50%
	rc.aa_upper = 54000; // ~66%
	init_timetype(&rc.start);
	init_timetype(&rc.end);
	init_timetype(&rc.ival);
	init_timetype(&rc.next_alarm);

	// inform user
	if (rc.format == DATA_FORMAT_ASCII) {
		reply("\nstart\n");
		reply("firmware: %s\n", __FIRMWARE_VERSION);
	}

	// read/write info from/to sd
	inform_SD_reset();
	read_config_from_SD(&rc);

	init_mode(&rc);
}

void run(void) {
	// this is called in a endless loop (!)

	/* Uart IR is enabled only during (light) sleep phases.
	 * If a uart IR occur during the IR is disabled, it will
	 * come up, immediately after enabling it agian.
	 * (see next line) */
	//=================================================================
	__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM); //
	if (!rxtx.wakeup && !rtc.alarmA_wakeup) {        //
		cpu_enter_LPM();
	}                                                                 //
	__HAL_UART_DISABLE_IT(&hrxtx, UART_IT_CM);                        //
	//=================================================================

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

uint32_t get_itime(uint8_t idx){
	if (rc.itime[idx] < 0) {
		return autoadjust_itime(rc.aa_lower, rc.aa_upper);
	}
	return rc.itime[idx];
}

static void extcmd_handler(void) {
	int32_t nr = 0, nr1 = 0;
	char *str = NULL;

	switch (extcmd.cmd) {

	/* RUN ============================================================ */

	case USR_CMD_SINGLE_MEASURE_START:
		if (get_itime(rc.itime_index) <= 0) {
			errreply("intergration time not set\n");
			break;
		}
		ok();
		sensor_init();
		sensor_measure(rc.itime[rc.itime_index]);
		sensor_deinit();
		send_data();
		break;

	case USR_CMD_MULTI_MEASURE_START:
		ok();
		multimeasure(false);
		break;


	/* GETTER ============================================================ */

	case USR_CMD_VERSION:
		reply("firmware: %s\n", __FIRMWARE_VERSION);
		break;

	case USR_CMD_GET_DATA:
		send_data();
		break;

	case USR_CMD_GET_ITIME:
		if (rc.format == DATA_FORMAT_BIN) {
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &(rc.itime[rc.itime_index]), 4, 1000);
		} else {
			reply("integration time [%u] = %ld us\n", rc.itime_index, rc.itime[rc.itime_index]);
		}
		break;

	case USR_CMD_GET_RTC_TIME:
		ts = rtc_get_now();
		if (rc.format == DATA_FORMAT_ASCII) {
			reply("20%02i-%02i-%02iT%02i:%02i:%02i\n", ts.date.Year, ts.date.Month, ts.date.Date, ts.time.Hours,
					ts.time.Minutes, ts.time.Seconds);
		} else {
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.date.Year, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.date.Month, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.date.Date, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.time.Hours, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.time.Minutes, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.time.Seconds, 1, 1000);
		}
		break;

	case USR_CMD_GET_CONFIG:
		print_config(&rc, "SYSTEM CONFIG");
		break;

		/* SETTER ============================================================ */

	case USR_CMD_SET_FORMAT:
		if (argparse_nr(&nr)) {
			fail();
			break;
		}
		/* check and set argument */
		rc.format = (nr > 0) ? DATA_FORMAT_ASCII : DATA_FORMAT_BIN;
		if(rc.format == DATA_FORMAT_BIN){
			rc.debuglevel = false;
		}
		ok();
		break;

	case USR_CMD_SET_ITIME:
		if (argparse_nr(&nr)) {
			fail();
			break;
		}

		// itime <  0: auto measure
		// itime == 0: disable index
		// itime >  0: check min/max
		if(nr < 0){
			nr = -1;
		} else if ( nr > 0){
			nr = MAX(MIN_INTERGATION_TIME, nr);
			nr = MIN(MAX_INTERGATION_TIME, nr);
		}
		rc.itime[rc.itime_index] = nr;
		ok();
		break;

	case USR_CMD_SET_ITIME_INDEX:
		if (argparse_nr(&nr)) {
			fail();
			break;
		}
		if (0 <= nr && nr < RCCONF_MAX_ITIMES) {
			rc.itime_index = nr;
			ok();
		}else{
			fail();
		}
		break;

	case USR_CMD_SET_MULTI_MEASURE_ITERATIONS:
		if (argparse_nr(&nr)) {
			fail();
			break;
		}
		if (0 < nr && nr < RCCONF_MAX_ITERATIONS) {
			rc.iterations = nr;
			ok();
		}else{
			fail();
		}
		break;

	case USR_CMD_SET_RTC_TIME:
		if (argparse_str(&str)) {
			fail();
			break;
		}
		if (rtc_parsecheck_datetime(str, &ts.time, &ts.date)) {
			fail();
			break;
		}
		/* store the current time */
		rtc_get_now_str(ts_buff, TS_BUFF_SZ);
		HAL_RTC_SetTime(&hrtc, &ts.time, RTC_FORMAT_BIN);
		HAL_RTC_SetDate(&hrtc, &ts.date, RTC_FORMAT_BIN);
		inform_SD_rtc(ts_buff);
		ok();
		break;

	case USR_CMD_SET_MODE:
		if (argparse_str(&str)) {
			fail();
			break;
		}
		if (parse_ival(str, &rc)) {
			fail();
			break;
		}
		init_mode(&rc);
		ok();
		break;

	case USR_CMD_SET_AUTOADJUST_PARAMS:
		if (argparse_nrs(&nr, &nr1)) {
			fail();
			break;
		}
		// constrains
		if (nr > UINT16_MAX || nr1 > UINT16_MAX || nr < 0 || nr1 < 0 || nr > nr1){
			fail();
			break;
		}
		rc.aa_lower = nr;
		rc.aa_upper = nr1;
		// no break

	case USR_CMD_TEST_AUTOADJUST:
		nr = autoadjust_itime(rc.aa_lower, rc.aa_upper);
		reply("upper, lower:  %ld, %ld\n", rc.aa_lower, rc.aa_upper);
		reply("test aa-itime: %ld us\n", nr);
		break;

		/* OTHER ============================================================ */

	case USR_CMD_HELP:
		reply(HELPSTR);
		break;

	case USR_CMD_STORE_SDCONFIG:
		nr = write_config_to_SD(&rc);
		if (!nr){
			ok();
		} else {
			errreply("write to SD faild\n");
		}
		break;

	case USR_CMD_PRINT_SDCONFIG:
			print_config_from_SD();
			break;

	case USR_CMD_READ_SDCONFIG:
		nr = read_config_from_SD(&rc);
		if (!nr){
			init_mode(&rc);
			ok();
		} else {
			errreply("read from SD faild\n");
		}
		break;

		/* DEBUG ============================================================ */

	case USR_CMD_DEBUG:
		if (argparse_nr(&nr)) {
			fail();
			break;
		}
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

/** Local helper for sending data via the uart interface. */
static void send_data(void) {
	char *errstr;
	uint32_t *rptr;
	uint16_t i = 0;
	uint8_t errcode = sens1.errc;

	switch (errcode) {
	case ERRC_NO_ERROR:
		errstr = "";
		break;
	case ERRC_TIMEOUT:
		errstr = "ERR: TIMEOUT. Is sensor plugged in ?\n";
		/* otehr reasons: "ADC/sensor not powered, physical connections bad\n"; */
		break;
	case ERRC_NO_EOS:
		errstr = "ERR: NO EOS. Something went wrong, please debug manually.\n";
		break;
	case ERRC_EOS_EARLY:
		errstr = "ERR: EOS EARLY. Something went wrong, please debug manually.\n";
		break;
	case ERRC_UNKNOWN:
		errstr = "ERR: UNKNOWN. Unknown error occurred.\n";
		break;
	default:
		errstr = "ERR: Not implemented.\n";
		break;
	}

	if (rc.format == DATA_FORMAT_BIN) {
		/*Send the errorcode nevertheless an error occurred or not.*/
		HAL_UART_Transmit(&hrxtx, (uint8_t *) &errcode, 2, 200);
		if (!errcode) {
			/* send data */
			HAL_UART_Transmit(&hrxtx, (uint8_t *) (sens1.data->wptr - MSPARAM_PIXEL),
			MSPARAM_PIXEL * 4,
			MSPARAM_PIXEL * 4 * 100);
		}
	} else { /* DATA_FORMAT_ASCII */
		if (errcode) {
			printf(errstr);
		} else {
#if DBG_SEND_ALL
			rptr = sens1.data->base;
#else
			rptr = (uint32_t *) (sens1.data->wptr - MSPARAM_PIXEL);
#endif
			while (rptr < sens1.data->wptr) {
				if (rptr == (sens1.data->wptr - MSPARAM_PIXEL)) {
					/* Pretty print some lines..*/
					printf("\n"DELIMITER_STR);
					printf("\n"HEADER_STR);
					i = 0;
				}

				/* Break and enumerate line after 10 values.*/
				if (i % 10 == 0) {
					printf("\n%03d   %05d ", i, (int) *rptr);
				} else {
					printf("%05d ", (int) *rptr);
				}
				rptr++;
				i++;
			}
			printf("\n"DELIMITER_STR"\n\n");
		}
	}
}

static void periodic_alarm_handler(void) {
	debug(1,"(alarm): Periodic alarm \n");
	RTC_TimeTypeDef new;
	rtc_timestamp_t ts;
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

	for (uint8_t i = 0; i < RCCONF_MAX_ITIMES; ++i) {

		itime = get_itime(i);

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

static void print_config_from_SD(void){
	runtime_config_t rc = {0,};
	if (read_config_from_SD(&rc)){
		errreply("(main) read failed\n");
		return;
	}
	rc.next_alarm.Hours = rc.next_alarm.Minutes = rc.next_alarm.Seconds = 99;
	print_config(&rc, "SD-CONFIG");
}

/* This function is used to test functions
 * or functionality under development.
 * Especially if the code is hard to reach
 * in the normal program flow. E.g. If the
 * code is executed by timer. */
static void dbg_test(void) {
	return;
}
