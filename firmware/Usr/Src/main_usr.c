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

static void send_data(void);
static void multimeasure(void);
static void periodic_alarm_handler(void);
static void extcmd_handler(void);
static void dbg_test(void);

static void ok(void);
static void print_config(runtime_config_t *rc);

static statemachine_config_t state;
static runtime_config_t rc;
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

static void run_init(void) {
	power_switch_EN(ON);
	usr_hw_init();

	state.format = DATA_FORMAT_ASCII;
	state.stream = false;
	state.toSD = true;

	memset(&ts, 0, sizeof(ts));
	memset(&rc, 0, sizeof(rc));
	init_timetype(&rc.start);
	init_timetype(&rc.end);
	init_timetype(&rc.ival);
	init_timetype(&rc.next_alarm);
	rc.iterations = 1;
	rc.itime[0] = DEFAULT_INTEGRATION_TIME;
	rc.mode = IVAL_OFF;

#if DBG_CODE
	/* Overwrites default value from usr_uart.c */
	rxtx.debug = true;
#endif

	if (state.format == DATA_FORMAT_ASCII) {
		reply("\nstart\n");
	}

	inform_SD_reset();
	read_config_from_SD(&rc);
	set_initial_alarm(&rc);
}

int run(void) {
	run_init();

	while (1) {

		/* Uart IR is enabled only during (light) sleep phases */
		//=================================================================
		__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM); //
		if (!rxtx.wakeup && !state.stream && !rtc.alarmA_wakeup) {        //
			power_switch_EN(OFF);
			cpu_enter_LPM();
			power_switch_EN(ON);
		}                                                                 //
		__HAL_UART_DISABLE_IT(&hrxtx, UART_IT_CM);                        //
		//=================================================================

		if (hrxtx.ErrorCode) {
			/* See stm32l4xx_hal_uart.h for ErrorCodes.
			 * Handle Uart Errors immediately because every further
			 * send (e.g. printf) would overwrite the Error Code.*/
			rxtx_restart_listening();
		}

		if (rtc.alarmA_wakeup) {
			rtc.alarmA_wakeup = false;
			periodic_alarm_handler();
			if (state.stream) {
				/* The handler has deinit the sensor,
				 * undo that now if we are in stream mode */
				sensor_init();
			}
		}

		if (state.stream) {
			sensor_measure();
			send_data();
			if (sens1.errc) {
				sensor_deinit();
#if IGNORE_ERRORS_IN_STREAM
				sensor_init();
#else
				state.stream = 0;
#endif
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
	return 0;
}

static void extcmd_handler(void) {
	uint32_t tmp = 0;
	char *str = NULL;

	switch (extcmd.cmd) {

	/* RUN ============================================================ */

	case USR_CMD_SINGLE_MEASURE_START:
		ok();
		sensor_init();
		sensor_measure();
		sensor_deinit();
		send_data();
		/* A single measurement during stream mode, end the stream mode. */
		state.stream = 0;
		break;

	case USR_CMD_MULTI_MEASURE_START:
		ok();
		state.toSD = false;
		multimeasure();
		state.toSD = true;
		break;

	case USR_CMD_STREAM_START:
		ok();
		sensor_init();
		state.stream = 1;
		break;

	case USR_CMD_STREAM_END:
		ok();
		sensor_deinit();
		state.stream = 0;
		break;

		/* GETTER ============================================================ */

	case USR_CMD_GET_DATA:
		send_data();
		break;

	case USR_CMD_GET_ITIME:
		if (state.format == DATA_FORMAT_BIN) {
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &sens1.itime, 4, 1000);
		} else {
			reply("integration time [0] = %lu us\n", sens1.itime);
		}
		break;

	case USR_CMD_GET_INDEXED_ITIME:
		tmp = rc.itime[rc.itime_index];
		if (state.format == DATA_FORMAT_BIN) {
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.itime_index, 4, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &tmp, 4, 1000);
		} else {
			reply("integration time [%u] = %lu us\n", rc.itime_index, tmp);
		}
		break;

	case USR_CMD_GET_RTC_TIME:
		ts = rtc_get_now();
		if (state.format == DATA_FORMAT_ASCII) {
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

	case USR_CMD_GET_INTERVAL:
		if (state.format == DATA_FORMAT_ASCII) {
			reply("interval mode: %u\n", rc.mode);
			reply("start time: %02i:%02i:%02i\n", rc.start.Hours, rc.start.Minutes, rc.start.Seconds);
			reply("end time:   %02i:%02i:%02i\n", rc.end.Hours, rc.end.Minutes, rc.end.Seconds);
			reply("ival:       %02i:%02i:%02i\n", rc.ival.Hours, rc.ival.Minutes, rc.ival.Seconds);
		} else {
			/* Transmit binary */
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.mode, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.ival.Hours, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.ival.Minutes, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.ival.Seconds, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.start.Hours, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.start.Minutes, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.start.Seconds, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.end.Hours, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.end.Minutes, 1, 1000);
			HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.end.Seconds, 1, 1000);
		}
		break;

	case USR_CMD_GET_CONFIG:
		print_config(&rc);
		break;

		/* SETTER ============================================================ */

	case USR_CMD_SET_FORMAT:
		if (argparse_nr(&tmp)) {
			break;
		}
		/* check and set argument */
		state.format = (tmp > 0) ? DATA_FORMAT_ASCII : DATA_FORMAT_BIN;
		if(state.format == DATA_FORMAT_BIN){
			rxtx.debug = false;
		}
		ok();
		break;

	case USR_CMD_SET_ITIME:
		if (argparse_nr(&tmp)) {
			break;
		}
		rc.itime[rc.itime_index] = tmp;
		if (rc.itime_index == 0) {
			/* 0 is the default itime, which is used for
			 * the single measurement command. */
			tmp = sensor_set_itime(tmp);
			/* if minimal integration time is underflown,
			 * the itime is set to the minmal possible(!) value.
			 * To keep the config consistent we need to update
			 * the default itime in slot 0. */
			rc.itime[0] = tmp;
		}
		ok();
		break;

	case USR_CMD_SET_ITIME_INDEX:
		if (argparse_nr(&tmp)) {
			break;
		}
		if (tmp < RCCONF_MAX_ITIMES) {
			rc.itime_index = tmp;
			ok();
		}
		break;

	case USR_CMD_SET_MULTI_MEASURE_ITERATIONS:
		if (argparse_nr(&tmp)) {
			break;
		}
		if (0 < tmp && tmp < RCCONF_MAX_ITERATIONS) {
			rc.iterations = tmp;
			ok();
		}
		break;

	case USR_CMD_SET_RTC_TIME:
		if (argparse_str(&str)) {
			break;
		}
		if (rtc_parsecheck_datetime(str, &ts.time, &ts.date)) {
			break; // todo: error message
		}
		/* store the current time */
		rtc_get_now_str(ts_buff, TS_BUFF_SZ);
		HAL_RTC_SetTime(&hrtc, &ts.time, RTC_FORMAT_BIN);
		HAL_RTC_SetDate(&hrtc, &ts.date, RTC_FORMAT_BIN);
		inform_SD_rtc(ts_buff);
		ok();
		break;

	case USR_CMD_SET_INTERVAL:
		if (argparse_str(&str)) {
			break;
		}
		if (parse_ival(str, &rc)) {
			break;
		}
		set_initial_alarm(&rc);
		ok();
		break;

		/* OTHER ============================================================ */

	case USR_CMD_HELP:
		reply(HELPSTR);
		break;

	case USR_CMD_STORE_CONFIG:
		tmp = write_config_to_SD(&rc);
		if (!tmp){
			ok();
		} else {
			printf("ERR: write to SD faild\n");
		}
		break;

	case USR_CMD_READ_CONFIG:
		tmp = read_config_from_SD(&rc);
		if (!tmp){
			set_initial_alarm(&rc);
			ok();
		} else {
			printf("ERR: read from SD faild\n");
		}
		break;

		/* DEBUG ============================================================ */

	case USR_CMD_DEBUG:
		rxtx.debug = rxtx.debug ? false : true;
		if (rxtx.debug) {
			reply("debug on\n");
		} else {
			reply("debug off\n");
		}
		break;

	case USR_CMD_DBGTEST:
		/* This call do nothing if DBG_CODE is not defined */
		dbg_test();
		ok();
		break;

	case USR_CMD_SET_SENSOR:
	case USR_CMD_UNKNOWN:
		reply("???\n");
		break;
	}
}

/** print 'ok' */
static void ok(void) {
	if (state.format == DATA_FORMAT_ASCII) {
		reply("ok\n");
	}
}

static void print_config(runtime_config_t *rc){
		rtc_timestamp_t ts;
		for (int i = 0; i < RCCONF_MAX_ITIMES; ++i) {
			if (rc->itime[i] != 0) {
				reply("itime[%u] = %lu\n", i, rc->itime[i]);
			}
		}
		reply("ii: %u  ('i=' set itime[%u])\n", rc->itime_index, rc->itime_index);
		reply("iter. per meas. [N]: %u\n", rc->iterations);
		reply("interval mode: %u\n", rc->mode);
		reply("start time:      %02i:%02i:%02i\n", rc->start.Hours, rc->start.Minutes, rc->start.Seconds);
		reply("end time:        %02i:%02i:%02i\n", rc->end.Hours, rc->end.Minutes, rc->end.Seconds);
		reply("interval:        %02i:%02i:%02i\n", rc->ival.Hours, rc->ival.Minutes, rc->ival.Seconds);
		reply("next auto-meas.: %02i:%02i:%02i\n", rc->next_alarm.Hours, rc->next_alarm.Minutes, rc->next_alarm.Seconds);
		ts = rtc_get_now();
		reply("now:  20%02i-%02i-%02iT%02i:%02i:%02i\n", ts.date.Year, ts.date.Month, ts.date.Date, ts.time.Hours,
				ts.time.Minutes, ts.time.Seconds);
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

	if (state.format == DATA_FORMAT_BIN) {
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
	debug("Periodic alarm \n");
	RTC_TimeTypeDef new;
	rtc_timestamp_t ts;
	ts = rtc_get_now();
	debug("now: 20%02i-%02i-%02iT%02i:%02i:%02i\n", ts.date.Year, ts.date.Month, ts.date.Date, ts.time.Hours,
			ts.time.Minutes, ts.time.Seconds);

	if (rc.mode == IVAL_OFF) {
		return;
	}

	/* set new alarm */
	new = rtc_time_add(&rc.next_alarm, &rc.ival);
	if (rc.mode == IVAL_ENDLESS) {
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
	multimeasure();

	debug("next: %02i:%02i:%02i\n", rc.next_alarm.Hours, rc.next_alarm.Minutes, rc.next_alarm.Seconds);
}

static void multimeasure(void) {
	int8_t res = 0;
	for (int i = 0; i < RCCONF_MAX_ITIMES; ++i) {
		if (rc.itime[i] == 0) {
			/* itime disabled */
			continue;
		}
		sensor_set_itime(rc.itime[i]);
		debug("itime[%u]=%lu\n", i, rc.itime[i]);

		/* Measure N times */
		for (int n = 0; n < rc.iterations; ++n) {

			debug("N: %u/%u\n", n + 1, rc.iterations);
			/* Generate timestamp */
			rtc_get_now_str(ts_buff, TS_BUFF_SZ);

			/* Make a measurement */
			sensor_init();
			sensor_measure();

			if (state.toSD && HAS_SD) {
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
#if DBG_CODE
	runtime_config_t rc;
	if (read_config_from_SD(&rc)){
		debug("read failed\n");
		return;
	}
	rc.next_alarm.Hours = rc.next_alarm.Minutes = rc.next_alarm.Seconds = 99;
	reply("config on SD:\n");
	print_config(&rc);

#else
	reply("not implemented\n");
#endif
	return;
}
