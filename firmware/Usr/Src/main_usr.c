/*
 * main_usr.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include "main_usr.h"
#include "power.h"
#include "rtc.h"
#include "rtc_usr.h"
#include "global_config.h"
#include "micro_spec.h"
#include "usart_usr.h"
#include "cmd_parser.h"
#include "tim_usr.h"
#include "sd_card.h"
#include "string.h"
#include "fatfs.h"
#include <stdio.h>

static void send_data(void);
static void multimeasure(void);
static void set_initial_alarm(void);
static void periodic_alarm_handler(void);
static RTC_TimeTypeDef get_closest_next_alarm(void);
static void dbg_test(void);

static int8_t argparse_nr(uint32_t *nr);
static int8_t argparse_str(char **str);
static int8_t parse_ival(char *str);

static void ok(void);

static uint8_t measurement_to_SD(void);
static void inform_SD_reset(void);
static void inform_SD_rtc(void);
static void read_config_from_SD(void);
static void write_config_to_SD(void);

static statemachine_config_t state;
static runtime_config_t rc;
static rtc_timestamp_t ts;

static filename_t fname;
FIL *f = &SDFile;

#define TS_BUFF_SZ	32
char ts_buff[TS_BUFF_SZ];

void init_timetype(RTC_TimeTypeDef *time){
	time->Hours = 99;
	time->Minutes = 99;
	time->Seconds = 99;
}

void usr_hw_init(void){
	/* sa. Errata 2.1.3. or Arm ID number 838869 */
	uint32_t *ACTLR = (uint32_t *)0xE000E008;
	*ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk;

	/* We enable the interrupts later when we need them*/
	HAL_NVIC_DisableIRQ(RXTX_IRQn);
	HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
	HAL_NVIC_DisableIRQ(EXTI2_IRQn);
	HAL_NVIC_DisableIRQ(TIM5_IRQn);

	rtc_init();
	rxtx_init();
	tim1_Init();
	tim2_Init();
	tim5_Init();

	NVIC_EnableIRQ(RXTX_IRQn);
	rxtx_restart_listening();

}

static void statemachine_init(void){

	state.format = DATA_FORMAT_ASCII;
	state.stream = false;
	state.toSD = true;

	memset(&fname, 0, sizeof(fname));
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
}

int main_usr(void) {
	uint8_t err = 0;
	uint32_t tmp = 0;
	char *str = NULL;

	usr_hw_init();
	statemachine_init();

	if (state.format == DATA_FORMAT_ASCII) {
		printf("\nstart\n");
	}

	// fixme: remove vvvvvvv

//	rc.ival.Hours = 0;
//	rc.ival.Minutes = 0;
//	rc.ival.Seconds = 20;
//	rc.mode = IVAL_ENDLESS;

	// fixme: remove ^^^^^^^

	inform_SD_reset();
	read_config_from_SD();
	set_initial_alarm();

	while (1) {
		err = 0;

		/* Uart IR is enabled only during sleep phases */
		//=================================================================
		__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM);                         //
		if (!rxtx.wakeup && !state.stream && !rtc.alarmA_wakeup) {        //
			power_switch_EN(OFF);
			cpu_enter_LPM();
			power_switch_EN(ON);
		}                                                                 //
		__HAL_UART_DISABLE_IT(&hrxtx, UART_IT_CM);                        //
		//=================================================================

		if (hrxtx.ErrorCode){
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
			if (err) {
				state.stream = 0;
				sensor_deinit();
			}
		}

		if (rxtx.wakeup) {
			parse_extcmd(rxtx_rxbuffer.base, rxtx_rxbuffer.size);

			rxtx.wakeup = false;
			rxtx.cmd_bytes = 0;
			rxtx_restart_listening();

			switch (extcmd.cmd) {

			/* RUN ============================================================ */

			case USR_CMD_SINGLE_MEASURE_START:
				ok();
				sensor_init();
				sensor_measure();
				send_data();
				sensor_deinit();
				/* A single measurement during stream mode, end the stream mode. */
				state.stream = 0;
				break;

			case USR_CMD_MULTI_MEASURE_START:
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
				sensor_deinit();
				state.stream = 0;
				ok();
				break;


			/* GETTER ============================================================ */

			case USR_CMD_GET_DATA:
				send_data();
				break;

			case USR_CMD_GET_ITIME:
				if (state.format == DATA_FORMAT_BIN) {
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &sens1.itime, 4, 1000);
				} else {
					printf("integration time [0] = %lu us\n", sens1.itime);
				}
				break;

			case USR_CMD_GET_INDEXED_ITIME:
				tmp = rc.itime[rc.itime_index];
				if (state.format == DATA_FORMAT_BIN) {
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &rc.itime_index, 4, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &tmp, 4, 1000);
				} else {
					printf("integration time [%u] = %lu us\n", rc.itime_index, tmp);
				}
				break;

			case USR_CMD_GET_RTC_TIME:
				ts = rtc_get_now();
				if (state.format == DATA_FORMAT_ASCII) {
					printf("20%02i-%02i-%02iT%02i:%02i:%02i\n", ts.date.Year, ts.date.Month, ts.date.Date, ts.time.Hours,
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
				// TODO start mode end ival
				if (state.format == DATA_FORMAT_ASCII) {
					printf("interval mode: %u\n", rc.mode);
					printf("start time: %02i:%02i:%02i\n", rc.start.Hours, rc.start.Minutes, rc.start.Seconds);
					printf("end time:   %02i:%02i:%02i\n", rc.end.Hours, rc.end.Minutes, rc.end.Seconds);
					printf("%02i:%02i:%02i\n", rc.ival.Hours, rc.ival.Minutes, rc.ival.Seconds);
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
				for (int i = 0; i < RCCONF_MAX_ITIMES; ++i) {
					if(rc.itime[i] != 0){
						printf("itime[%u] = %lu\n", i, rc.itime[i]);
					}
				}
				printf("itime[%u] is currently choosen for setting.\n", rc.itime_index);
				printf("iteration per measurement: %u\n", rc.iterations);
				printf("interval mode: %u\n", rc.mode);
				printf("start time: %02i:%02i:%02i\n", rc.start.Hours, rc.start.Minutes, rc.start.Seconds);
				printf("end time:   %02i:%02i:%02i\n", rc.end.Hours, rc.end.Minutes, rc.end.Seconds);
				printf("interval:   %02i:%02i:%02i\n", rc.ival.Hours, rc.ival.Minutes, rc.ival.Seconds);
				printf("next alarm: %02i:%02i:%02i\n", rc.next_alarm.Hours, rc.next_alarm.Minutes, rc.next_alarm.Seconds);
				ts = rtc_get_now();
				printf("now: 20%02i-%02i-%02iT%02i:%02i:%02i\n", ts.date.Year, ts.date.Month, ts.date.Date, ts.time.Hours,
							ts.time.Minutes, ts.time.Seconds);
				break;

			/* SETTER ============================================================ */

			case USR_CMD_SET_FORMAT:
				/* parse argument */
				if (extcmd.arg_buffer[0] == 0) {
					tmp = 1;
				} else {
					sscanf(extcmd.arg_buffer, "%lu", &tmp);
				}
				/* check and set argument */
				state.format = (tmp > 0) ? DATA_FORMAT_ASCII : DATA_FORMAT_BIN;
				ok();
				break;

			case USR_CMD_SET_ITIME:
				if(argparse_nr(&tmp)){
					break;
				}
				rc.itime[rc.itime_index] = tmp;
				if(rc.itime_index == 0){
					/* 0 is the default itime, which is used for
					 * the single measurement command. */
					tmp = sensor_set_itime(tmp);
					/* if minimal inntergartion time is underflown,
					 * the itime is set to the minmal possible(!) value.
					 * To keep the config consistent we need to update
					 * the default itime in slot 0. */
					rc.itime[0] = tmp;
				}
				ok();
				break;

			case USR_CMD_SET_ITIME_INDEX:
				if(argparse_nr(&tmp)){
					break;
				}
				if(tmp < RCCONF_MAX_ITIMES){
					rc.itime_index = tmp;
					ok();
				}
				break;

			case USR_CMD_SET_MULTI_MEASURE_ITERATIONS:
				if(argparse_nr(&tmp)){
					break;
				}
				if(0 < tmp && tmp < RCCONF_MAX_ITERATIONS){
					rc.iterations = tmp;
					ok();
				}
				break;

			case USR_CMD_SET_RTC_TIME:
				if (argparse_str(&str)) {
					break;
				}
				err = rtc_parsecheck_datetime(str, &ts.time, &ts.date);
				if (err) {
					break; // todo: error message
				}
				/* store the current time */
				rtc_get_now_str(ts_buff, TS_BUFF_SZ);
				HAL_RTC_SetTime(&hrtc, &ts.time, RTC_FORMAT_BIN);
				HAL_RTC_SetDate(&hrtc, &ts.date, RTC_FORMAT_BIN);
				inform_SD_rtc();
				ok();
				break;

			case USR_CMD_SET_INTERVAL:
				if (argparse_str(&str)) {
					break;
				}
				err = parse_ival(str);
				if (err) {
					break;
				}
				set_initial_alarm();
				write_config_to_SD();
				ok();
				break;

			/* DEBUG ============================================================ */

			case USR_CMD_DEBUG:
				rxtx.debug = rxtx.debug ? false : true;
				if (rxtx.debug) {
					printf("debug on\n");
				} else {
					printf("debug off\n");
				}
				break;

			case USR_CMD_DBGTEST:
				/* This call do nothing if DBG_CODE is not defined */
				dbg_test();
				break;

			case USR_CMD_SET_SENSOR:
			case USR_CMD_UNKNOWN:
				break;

			}
		}
	}
	return 0;
}

/**
 * Parse 'mode,ival,start,end' from string and
 * set the aprropiate params in rc-struct.
 * If parsing fails, rc-struct is untouched.
 *
 * Return 0 on success,
 * Return 1 on parsing error
 * Return 2 on constrains error
 *
 * Note: Start time needs to be smaller than end time
 */
static int8_t parse_ival(char *str){

	uint c = 99;
	RTC_TimeTypeDef iv, st, en, off;
	off.Hours = 99; off.Minutes = 99, off.Seconds =99;

	/*parse first nr - alarm on/off*/
	sscanf(str, "%u", &c);
	if(c == 0){
		rc.start = off;
		rc.end = off;
		rc.ival = off;
		rc.mode = IVAL_OFF;
		return 0;
	}
	if(c != 1 && c != 2){
		return 1;
	}
	str++; //ignore c

	/*parse interval*/
	str = (char*) memchr(str, ',', 5);
	if(!str){
		return 1;
	}
	str++; // ignore ','
	if(rtc_parse_time(str, &iv)){
		return 1;
	}

	if (iv.Hours == 0 && iv.Minutes == 0 && iv.Seconds < MIN_IVAL) {
		return 1;
	}
	if(c == 2){
		rc.ival = iv;
		rc.mode = IVAL_ENDLESS;
		return 0;
	}

	/*parse start-time*/
	str = (char*) memchr(str, ',', 20);
	if(!str){
		return 1;
	}
	str++; // ignore ','
	if(rtc_parse_time(str, &st)){
		return 1;
	}

	/*parse end-time*/
	str = (char*) memchr(str, ',', 20);
	if(!str){
		return 1;
	}
	str++; // ignore ','
	if(rtc_parse_time(str, &en)){
		return 1;
	}

	/*check constrains*/
	if (!rtc_time_lt(&st, &en)){
		return 2;
	}

	rc.ival = iv;
	rc.start = st;
	rc.end = en;
	rc.mode = IVAL_STARTEND;
	return 0;
}

/** print 'ok' */
static void ok(void) {
	if (state.format == DATA_FORMAT_ASCII) {
		printf("ok\n");
	}
}

/** Parse natural number to arg.
 *  Return 0 on success, otherwise non-zero */
static int8_t argparse_nr(uint32_t *nr) {
	int res;
	if (extcmd.arg_buffer[0] == 0) {
		/* buffer empty */
		return -1;
	}
	res = sscanf(extcmd.arg_buffer, "%lu", nr);
	if (res <= 0) {
		nr = 0;
		return res;
	}
	return 0;
}

/** Parse string to arg.
 *  Return 0 on success, otherwise non-zero */
static int8_t argparse_str(char **str) {
	UNUSED(str);
	if (extcmd.arg_buffer[0] == 0) {
		/* buffer empty */
		return -1;
	}
	*str = extcmd.arg_buffer;
	return 0;
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

static void inform_SD_reset(void) {
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

static void inform_SD_rtc(void) {
#if HAS_SD
	uint8_t res = 0;
	res = sd_mount();
	if (!res) {
		res = sd_find_right_filename(fname.postfix, &fname.postfix, fname.buf, FNAME_BUF_SZ);
		if (!res) {
			res = sd_open_file_neworappend(f, fname.buf);
			if (!res) {
				f_printf(f, "The RTC was set. Old time: %S\n", ts_buff);
				rtc_get_now_str(ts_buff, TS_BUFF_SZ);
				f_printf(f, "                 New time: %S\n", ts_buff);
				sd_close(f);
			}
		}
		sd_umount();
	}
#else
	return;
#endif
}

static void write_config_to_SD(void) {
#if HAS_SD
	uint8_t res = 0;
	res = sd_mount();
	if (!res) {
		res = sd_open(f, SD_CONFIGFILE_NAME, FA_WRITE | FA_CREATE_ALWAYS);
		if (!res) {
			f_printf(f, "%U\n", RCCONF_MAX_ITIMES);
			for (int i = 0; i < RCCONF_MAX_ITIMES; ++i) {
				f_printf(f, "%LU\n", rc.itime[i]);
			}
			f_printf(f, "%U\n", rc.iterations);

			/* We store the ival like the user command format:
			 * 'mode,ival,start,end'
			 * e.g.: '1,00:00:20,04:30:00,22:15:00*/
			f_printf(f, "%U,", rc.mode);
			f_printf(f, "%02U:%02U:%02U,", rc.ival.Hours, rc.ival.Minutes, rc.ival.Seconds);
			f_printf(f, "%02U:%02U:%02U,", rc.start.Hours, rc.start.Minutes, rc.start.Seconds);
			f_printf(f, "%02U:%02U:%02U\n", rc.end.Hours, rc.end.Minutes, rc.end.Seconds);
			sd_close(f);
		}
		sd_umount();
	}
#else
	return;
#endif
}

static void read_config_from_SD(void){
#if HAS_SD
# if RCCONF_MAX_ITIMES > 32
# error "Attention buffer gets big.. Improve implementation :)"
# endif
	/* max value of itime = 10000\n -> 6 BYTES * RCCONF_MAX_ITIMES
	 * start end ival timestamp     -> 3 * 20 BYTES
	 * three other number good will aprox. -> 20 BYTES*/
	uint16_t sz = RCCONF_MAX_ITIMES*6 + 3*20 + 20;
	uint8_t buf[sz];
	uint8_t res;
	uint32_t nr, rcconf_max_itimes;
	uint bytesread;
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
					rc.itime[i] = nr;
				}
			}
			if (!fail) {
				/* Read iterations aka. N from sd*/
				token = strtok_r(rest, "\n", &rest);
				res = sscanf(token, "%lu", &nr);
				if (token != NULL && res > 0 && nr > 0) {
					rc.iterations = nr;
					/* Read 'mode,ival,start,end' as one string from sd.
					 * If parse_ival() fails, no times are set. */
					token = rest;
					parse_ival(token);
				}
			}
			sd_close(f);
		}
		sd_umount();
	}
#else
	return;
#endif
}

/** Store the measurment data to SD card.
 * Requires a mounted SD card. */
static uint8_t measurement_to_SD(void){
#if HAS_SD
	int8_t res = 0;
	res = sd_find_right_filename(fname.postfix, &fname.postfix, fname.buf, FNAME_BUF_SZ);
	if (!res) {
		res = sd_open_file_neworappend(f, fname.buf);
		if (!res) {
			/* Write metadata (timestamp, errorcode, intergartion time) */
			f_printf(f, "%S, %U, %LU, [,", ts_buff, sens1.errc, sens1.itime);
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
			if (rxtx.debug) {
				/* Use printf() instead of debug() to prevent 'dbg:' string before every value. */
				printf("%s, %u, %lu, [,", ts_buff, sens1.errc, sens1.itime);
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
	return 100;
#endif
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

			debug("N: %u/%u\n", n+1, rc.iterations);
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
					res = measurement_to_SD();
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

static void periodic_alarm_handler(void) {
	debug("Periodic alarm \n");
	RTC_TimeTypeDef new;
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

/**
 * Calculate and return the next alarm closest to now.
 *
 * Note: Call this ONLY in IVAL_STARTEND mode, otherwise
 * stuff break !
 */
static RTC_TimeTypeDef get_closest_next_alarm(void) {
	uint32_t start, end, now, ival, N, x;
	rtc_timestamp_t t;
	t = rtc_get_now();
	now = rtc_time2seconds(&t.time);
	ival = rtc_time2seconds(&rc.ival);
	start = rtc_time2seconds(&rc.start);
	end = rtc_time2seconds(&rc.end);
	/*Safety window of 2 seconds. One is not enough
	 * because we could be very close to a second transition.*/
	now = now + 2;
	/* N <= n < N+1, where n is the correct float-value */
	N = (now - start) / ival;
	x = (start + N * ival > now) ? N * ival : (N + 1) * ival;
	x = (start + x <= end) ? start + x : start;
	return rtc_seconds2time(x);
}

/*
 * Set the alarm if the user was submitting a new value.
 */
static void set_initial_alarm(void) {
	RTC_TimeTypeDef t;
	if (rc.mode == IVAL_OFF) {
		init_timetype(&rc.ival);
		init_timetype(&rc.next_alarm);
		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);

	} else if (rc.mode == IVAL_STARTEND) {
		t = get_closest_next_alarm();
		rtc_set_alarmA(&t);
		rc.next_alarm = rtc_get_alermAtime();

	} else if (rc.mode == IVAL_ENDLESS) {
		ts = rtc_get_now();
		rtc_set_alarmA_by_offset(&ts.time, &rc.ival);
		rc.next_alarm = rtc_get_alermAtime();
	}
}

static bool isON = false;
/* This function is used to test functions
 * or functionality under development.
 * Especially if the code is hard to reach
 * in the normal program flow. E.g. If the
 * code is executed by timer. */
static void dbg_test(void) {
#if DBG_CODE

	if(isON){
		HAL_GPIO_WritePin(POWER5V_SWITCH_ENBL_GPIO_Port, POWER5V_SWITCH_ENBL_Pin, GPIO_PIN_RESET);
		printf("powerswitch OFF\n");
	} else {
		HAL_GPIO_WritePin(POWER5V_SWITCH_ENBL_GPIO_Port, POWER5V_SWITCH_ENBL_Pin, GPIO_PIN_SET);
		printf("powerswitch ON\n");
	}
	isON = !isON;
	return;

	uint16_t sz = RCCONF_MAX_ITIMES*6 + 3*20 + 20;
	uint8_t buf[sz];
	uint8_t res;
	uint bytesread;

	memset(buf, 0, sz * sizeof(uint8_t));
	res = sd_mount();
	if (!res) {
		res = sd_open(f, SD_CONFIGFILE_NAME, FA_READ);
		if (!res) {
			f_read(f, buf, sizeof(buf), &bytesread);
			printf("Config-file on SD:\n%s", buf);
		}
	}
#endif
	return;
}
