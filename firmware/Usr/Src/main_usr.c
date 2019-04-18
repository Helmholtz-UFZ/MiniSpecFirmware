/*
 * main_usr.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include "main_usr.h"
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
static void periodic_alarm_handler(void);
static uint8_t measurement_to_SD(void);
static void dbg_test(void);

static time_config_t tconf;
static measure_config_t mconf;
static statemachine_config_t state;
static rtc_timestamp_t ts;

static filename_t fname;
FIL *f = &SDFile;

#define TS_BUFF_SZ	32
char ts_buff[TS_BUFF_SZ];

int main_usr(void) {
	uint8_t err = 0;
	uint32_t tmp = 0;
	uint8_t res = 0;
	char *str;

	state.format = DATA_FORMAT_ASCII;
	state.stream = false;

	memset(&fname, 0, sizeof(fname));
	memset(&tconf, 0, sizeof(tconf));
	memset(&mconf, 0, sizeof(mconf));
	memset(&ts, 0, sizeof(ts));

#if DBG_CODE
	/* Overwrites default value from usr_uart.c */
	tx_dbgflg = 1;
#endif

	/* We enable the interrupts later */
	HAL_NVIC_DisableIRQ( RXTX_IRQn);
	HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
	HAL_NVIC_DisableIRQ(EXTI2_IRQn);

	rxtx_init();
	tim1_Init();
	tim2_Init();
	tim5_Init();

	if (state.format == DATA_FORMAT_ASCII) {
		printf("\nstart\n");
	}

#if HAS_SD
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
#endif

	/* Run the system ------------------------------------------------------------*/

	// enabling usart receiving
	NVIC_EnableIRQ( RXTX_IRQn);
	HAL_UART_Receive_DMA(&hrxtx, rxtx_rxbuffer.base, rxtx_rxbuffer.size);

	while (1) {
		err = 0;

		// IR in uart module
		__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM);

		if (!rxtx.wakeup && !state.stream && !rtc_alarmA_occured) {
			cpu_enter_sleep_mode();
		}

		if (hrxtx.ErrorCode){
			/* See stm32l4xx_hal_uart.h for ErrorCodes.
			 * Handle Uart Errors immediately because every further
			 * send (e.g. printf) will overwrite the Error Code.*/
			rxtx_restart_listening();
		}

		// redundant in non-stream mode as also disabled in its ISR
		__HAL_UART_DISABLE_IT(&hrxtx, UART_IT_CM);

		if (rtc_alarmA_occured) {
			rtc_alarmA_occured = 0;
			periodic_alarm_handler();
			if (state.stream) {
				/* the handler has deinit the sensor,
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

			case USR_CMD_SINGLE_MEASURE_START:
				if (state.format == DATA_FORMAT_ASCII)
					printf("ok\n");
				sensor_init();
				sensor_measure();
				send_data();
				sensor_deinit();
				/* A single measurement during stream mode, end the stream mode. */
				state.stream = 0;
				break;

			case USR_CMD_GET_DATA:
				send_data();
				break;

			case USR_CMD_WRITE_ITIME:
				/* parse argument */
				if (extcmd.arg_buffer[0] == 0) {
					break;
				} else {
					sscanf(extcmd.arg_buffer, "%lu", &tmp);
				}
				/* check and set argument */
				sensor_set_itime(tmp);
				if (state.format == DATA_FORMAT_ASCII)
					printf("ok\n");
				break;

			case USR_CMD_READ_ITIME:
				if (state.format == DATA_FORMAT_BIN) {
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &sens1.itime, 4, 1000);
				} else {
					printf("integration time = %lu us\n", sens1.itime);
				}
				break;

			case USR_CMD_STREAM_START:
				if (state.format == DATA_FORMAT_ASCII)
					printf("ok\n");
				sensor_init();
				state.stream = 1;
				break;

			case USR_CMD_STREAM_END:
				sensor_deinit();
				state.stream = 0;
				if (state.format == DATA_FORMAT_ASCII)
					printf("ok\n");
				break;

			case USR_CMD_SET_FORMAT:
				/* parse argument */
				if (extcmd.arg_buffer[0] == 0) {
					tmp = 1;
				} else {
					sscanf(extcmd.arg_buffer, "%lu", &tmp);
				}
				/* check and set argument */
				state.format = (tmp > 0) ? DATA_FORMAT_ASCII : DATA_FORMAT_BIN;
				if (state.format == DATA_FORMAT_ASCII)
					printf("ok\n");
				break;

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

			case USR_CMD_GET_RTC_TIME:
				/* Always call GetDate after GetTime ! see HAL-documentation */
				HAL_RTC_GetTime(&hrtc, &ts.time, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &ts.date, RTC_FORMAT_BIN);

				if (state.format == DATA_FORMAT_ASCII) {
					printf("20%02i-%02i-%02iT%02i:%02i:%02i\n", ts.date.Year, ts.date.Month, ts.date.Date, ts.time.Hours,
							ts.time.Minutes, ts.time.Seconds);
				} else {
					/* Transmit binary */
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.date.Year, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.date.Month, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.date.Date, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.time.Hours, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.time.Minutes, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &ts.time.Seconds, 1, 1000);
				}
				break;

			case USR_CMD_SET_RTC_TIME:
				if (extcmd.arg_buffer[0] == 0) {
					break;
				} else {
					str = extcmd.arg_buffer;
				}
				err = rtc_parse_datetime(str, &ts.time, &ts.date);
				if (err) {
					break;
				}
				/* store the current time */
				rtc_get_now_str(ts_buff, TS_BUFF_SZ);
				HAL_RTC_SetTime(&hrtc, &ts.time, RTC_FORMAT_BIN);
				HAL_RTC_SetDate(&hrtc, &ts.date, RTC_FORMAT_BIN);
				/* If interval was set also update it. If it is disabled (all values zero)
				 * than the call will have no effect.*/
				rtc_set_alarmA_by_offset(&ts.time, &tconf.ival);
#if HAS_SD
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
#endif
				if (state.format == DATA_FORMAT_ASCII)
					printf("ok\n");
				break;

			case USR_CMD_GET_INTERVAL:
				if (state.format == DATA_FORMAT_ASCII) {
					printf("%02i:%02i:%02i\n", tconf.ival.Hours, tconf.ival.Minutes, tconf.ival.Seconds);
				} else {
					/* Transmit binary */
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &tconf.ival.Hours, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &tconf.ival.Minutes, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &tconf.ival.Seconds, 1, 1000);
				}
				break;

			case USR_CMD_SET_INTERVAL:
				if (extcmd.arg_buffer[0] == 0) {
					break;
				} else {
					str = extcmd.arg_buffer;
				}
				err = rtc_parse_interval(str, &ts.time);
				if (err) {
					break;
				}
				if (ts.time.Hours == 0 && ts.time.Minutes == 0 && ts.time.Seconds == 0) {
					/* All zero deactivates the periodically alarm, so this is a valid case.*/
					;
				} else if (ts.time.Hours == 0 && ts.time.Minutes == 0 && ts.time.Seconds < MIN_IVAL) {
					/* Ensure that the interval is long enough to operate safely.*/
					break;
				}
				/* if all ok update the rtc ival variable which periodically updated the alarmA. */
				tconf.ival.Hours = ts.time.Hours;
				tconf.ival.Minutes = ts.time.Minutes;
				tconf.ival.Seconds = ts.time.Seconds;

				/* Get the current time (ignore date, but always call both) */
				HAL_RTC_GetTime(&hrtc, &ts.time, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &ts.date, RTC_FORMAT_BIN);

				/* and finally set the alarm.*/
				rtc_set_alarmA_by_offset(&ts.time, &tconf.ival);

				if (state.format == DATA_FORMAT_ASCII)
					printf("ok\n");
				break;

			case USR_CMD_SET_MULTI_MEASURE_NR:
				break;

			default:
				break;
			}
		}
	}
	return 0;
}

/**
 * Local helper for sending data via the uart interface.
 */
static void send_data(void) {
	char *errstr;
	uint16_t *rptr;
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
			MSPARAM_PIXEL * 2,
			MSPARAM_PIXEL * 2 * 100);
		}
	} else { /* DATA_FORMAT_ASCII */
		if (errcode) {
			printf(errstr);
		} else {
#if DBG_SEND_ALL
			rptr = sens1.data->base;
#else
			rptr = (uint16_t *) (sens1.data->wptr - MSPARAM_PIXEL);
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
					printf("\n%03d   %05d ", i, *rptr);
				} else {
					printf("%05d ", *rptr);
				}
				rptr++;
				i++;
			}
			printf("\n"DELIMITER_STR"\n\n");
		}
	}
}

/**
 * @brief cpu_sleep()
 * Disabel SysTick and make CPU enter sleep-mode.
 *
 * @sa cpu_awake()
 */
void cpu_enter_sleep_mode(void) {
	// to prevent wakeup by Systick interrupt.
	HAL_SuspendTick();
	// go back to sleep after handling an IR
	// cleared by calling cpu_enter_run_mode() from ISR
	HAL_PWR_EnableSleepOnExit();
	//sleep + WaintForInterrupt-----------------------------------------
	HAL_PWR_EnterSLEEPMode( PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	//awake again ------------------------------------------------------
	HAL_ResumeTick();
}

/**
 * @brief cpu_awake()
 * EXECUTED BY ISR - KEEP IT SHORT
 */
void cpu_enter_run_mode(void) {
	// wake up after handling the actual IR
	CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPONEXIT_Msk));
}

/** Store the measurment data to SD card.
 * Requires a mounted SD card. */
static uint8_t measurement_to_SD(void){
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
				uint16_t *p = (uint16_t *) (sens1.data->wptr - MSPARAM_PIXEL);
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
				uint16_t *p = (uint16_t *) (sens1.data->wptr - MSPARAM_PIXEL);
				for (uint16_t i = 0; i < MSPARAM_PIXEL; ++i) {
					printf("%u,", *(p++));
				}
				printf("]\n");
			}
		}
	}
	return res;
}

static void periodic_alarm_handler(void) {
	/* For each integration time, measure N times and store to SD */
	RTC_AlarmTypeDef a;
	int8_t res = 0;
	uint8_t N = 1;
	debug("Periodic alarm \n");

	/* Set the alarm to new time according to the interval value.*/
	HAL_RTC_GetAlarm(&hrtc, &a, RTC_ALARM_A, RTC_FORMAT_BIN);
	rtc_set_alarmA_by_offset(&a.AlarmTime, &tconf.ival);

	/* Set integration time todo*/
//	sens1.itime = 0;

	/* Measure N times */
	for (int n = 0; n < N; ++n) {

		/* Generate timestamp */
		rtc_get_now_str(ts_buff, TS_BUFF_SZ);

		/* Make a measurement */
		sensor_init();
		sensor_measure();

#if HAS_SD
		/* Write measurement to SD */
		res = sd_mount();
		if (!res) {
			/* Store the measurement on SD */
			res = measurement_to_SD();
			sd_umount();
		}
#endif
		if(sens1.errc){
			sensor_deinit();
		}
	}
}

/* This function is used to test functions
 * or functionality under development.
 * Especially if the code is hard to reach
 * in the normal program flow. E.g. If the
 * code is executed by timer. */
static void dbg_test(void) {
#if DBG_CODE
	periodic_alarm_handler();
#endif
	return;
}
