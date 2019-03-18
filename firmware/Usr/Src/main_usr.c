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
#include "tim_usr.h"
#include "sd_card.h"
#include "string.h"
#include "fatfs.h"
#include <stdio.h>

static void send_data(uint8_t sens_status, uint8_t format);
static void parse_extcmd(uint8_t *buffer, uint16_t size);
static void periodic_alarm_handler(void);
static uint32_t map_status2errcode(uint8_t status);
static usr_cmd_typedef extcmd;
static uint8_t last_sensor_status = 0;

static uint8_t data_format = DATA_FORMAT_ASCII;
static bool stream_mode = 0;

#define fname_buf_sz 	128
char fname_buf[fname_buf_sz];
uint16_t fname_curr_postfix = 0;

static RTC_DateTypeDef sDate;
static RTC_TimeTypeDef sTime;

int main_usr(void) {
	uint8_t err = 0;
	uint32_t tmp;
	char *str;

	/* We enable the interrupts later */
	HAL_NVIC_DisableIRQ( RXTX_IRQn);
	HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
	HAL_NVIC_DisableIRQ(EXTI2_IRQn);

	rxtx_init();
	tim1_Init();
	tim2_Init();
	tim5_Init();

#if HAS_SD
	/* Inform the File that an reset occurred */
	uint8_t res;
	UNUSED(res);
	res = sd_mount();
	res = sd_find_right_filename(fname_curr_postfix, &fname_curr_postfix, fname_buf, fname_buf_sz);
	res = sd_write_file(fname_buf, "\nThe sensor was reset/powered-down.\n");
	res = sd_umount();
#endif

	/* Run the system ------------------------------------------------------------*/

	// enabling usart receiving
	NVIC_EnableIRQ( RXTX_IRQn);
	HAL_UART_Receive_DMA(&hrxtx, rxtx_rxbuffer.base, rxtx_rxbuffer.size);

	if (data_format == DATA_FORMAT_ASCII) {
		printf("\nstart\n");
	}
	while (1) {
		err = 0;

		// IR in uart module
		__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM);

		if (rxtx_CR_recvd == 0 && stream_mode == 0 && rtc_alarmA_occured == 0) {
			cpu_enter_sleep_mode();
		}

		// redundant in non-stream mode as also disabled in its ISR
		__HAL_UART_DISABLE_IT(&hrxtx, UART_IT_CM);

		if (rtc_alarmA_occured) {
			rtc_alarmA_occured = 0;
			periodic_alarm_handler();
			if (stream_mode) {
				/* the handler has deinit the sensor,
				 * undo that now if we are in stream mode */
				sensor_init();
			}
		}

		if (stream_mode) {
			err = sensor_measure();
			last_sensor_status = sens1.status;
			send_data(last_sensor_status, data_format);
			if (err) {
				stream_mode = 0;
				sensor_deinit();
			}
		}

		if (rxtx_CR_recvd) {
			parse_extcmd(rxtx_rxbuffer.base, rxtx_rxbuffer.size);

			rxtx_CR_recvd = 0;
			rxtx_cmd_bytes = 0;
			rxtx_restart_listening();

			switch (extcmd.cmd) {

			case USR_CMD_SINGLE_MEASURE_START:
				if (data_format == DATA_FORMAT_ASCII)
					printf("ok\n");
				sensor_init();
				sensor_measure();
				last_sensor_status = sens1.status;
				send_data(last_sensor_status, data_format);
				sensor_deinit();
				/* A single measurement in stream mode end stream mode. */
				stream_mode = 0;
				break;

			case USR_CMD_GET_DATA:
				send_data(last_sensor_status, data_format);
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
				if (data_format == DATA_FORMAT_ASCII)
					printf("ok\n");
				break;

			case USR_CMD_READ_ITIME:
				if (data_format == DATA_FORMAT_BIN) {
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &sens1.itime, 4, 1000);
				} else {
					printf("integration time = %lu us\n", sens1.itime);
				}
				break;

			case USR_CMD_STREAM_START:
				if (data_format == DATA_FORMAT_ASCII)
					printf("ok\n");
				sensor_init();
				stream_mode = 1;
				break;

			case USR_CMD_STREAM_END:
				sensor_deinit();
				stream_mode = 0;
				if (data_format == DATA_FORMAT_ASCII)
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
				data_format = (tmp > 0) ? DATA_FORMAT_ASCII : DATA_FORMAT_BIN;
				if (data_format == DATA_FORMAT_ASCII)
					printf("ok\n");
				break;

			case USR_CMD_DEBUG:
				//			testtest();
				tx_dbgflg = tx_dbgflg ? 0 : 1;
				if (tx_dbgflg) {
					printf("debug on\n");
				} else {
					printf("debug off\n");
				}
				break;

			case USR_CMD_GET_RTC_TIME:
				/* Always call GetDate after GetTime ! see HAL-documentation */
				HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

				if (data_format == DATA_FORMAT_ASCII) {
					printf("20%02i-%02i-%02iT%02i:%02i:%02i\n",
							sDate.Year, sDate.Month, sDate.Date,
							sTime.Hours, sTime.Minutes, sTime.Seconds);
				} else {
					/* Transmit binary */
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &sDate.Year, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &sDate.Month, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &sDate.Date, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &sTime.Hours, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &sTime.Minutes, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &sTime.Seconds, 1, 1000);
				}
				break;

			case USR_CMD_SET_RTC_TIME:
				if (extcmd.arg_buffer[0] == 0) {
					break;
				} else {
					str = extcmd.arg_buffer;
				}
				err = rtc_parse_datetime(str, &sTime, &sDate);
				if (err) {
					break;
				}
				HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
				HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
				/* If interval was set also update it. If it is disabled (all values zero)
				 * than the call will have no effect.*/
				rtc_set_alarmA_by_offset(&sTime, &rtc_ival);

				if (data_format == DATA_FORMAT_ASCII)
					printf("ok\n");
				break;

			case USR_CMD_GET_INTERVAL:
				if (data_format == DATA_FORMAT_ASCII) {
					printf("%02i:%02i:%02i\n", rtc_ival.Hours, rtc_ival.Minutes,
							rtc_ival.Seconds);
				} else {
					/* Transmit binary */
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &rtc_ival.Hours, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &rtc_ival.Minutes, 1, 1000);
					HAL_UART_Transmit(&hrxtx, (uint8_t *) &rtc_ival.Seconds, 1, 1000);
				}
				break;

			case USR_CMD_SET_INTERVAL:
				if (extcmd.arg_buffer[0] == 0) {
					break;
				} else {
					str = extcmd.arg_buffer;
				}
				err = rtc_parse_interval(str, &sTime);
				if (err) {
					break;
				}
				if (sTime.Hours == 0 && sTime.Minutes == 0 && sTime.Seconds == 0) {
					/* All zero deactivates the periodically alarm, so this is a valid case.*/
					;
				} else if (sTime.Hours == 0 && sTime.Minutes == 0 && sTime.Seconds < MIN_IVAL) {
					/* Ensure that the interval is long enough to operate safely.*/
					break;
				}
				/* if all ok update the rtc ival variable which periodically updated the alarmA. */
				rtc_ival.Hours = sTime.Hours;
				rtc_ival.Minutes = sTime.Minutes;
				rtc_ival.Seconds = sTime.Seconds;

				/* Get the current time (ignore date, but always call both) */
				HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

				/* and finally set the alarm.*/
				rtc_set_alarmA_by_offset(&sTime, &rtc_ival);

				if (data_format == DATA_FORMAT_ASCII)
					printf("ok\n");
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
 *
 * \param format	0 (DATA_FORMAT_BIN) send raw data, byte per byte.(eg. (dez) 1000 -> 0xE8 0x03)\n
 * \param format	1 (DATA_FORMAT_ASCII) send the data as in ASCII, as human readable text. (eg. (dez) 1000 -> '1' '0' '0' '0')
 */
static void send_data(uint8_t sens_status, uint8_t format) {
	char *errstr;
	uint32_t errcode = ERRC_NO_ERROR;
	uint16_t *rptr;
	uint16_t i = 0;

	errcode = map_status2errcode(sens_status);

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

	if (format == DATA_FORMAT_BIN) {
		/*Send the errorcode nevertheless an error occurred or not.*/
		HAL_UART_Transmit(&hrxtx, (uint8_t *) &errcode, 2, 200);
		if (errcode == ERRC_NO_ERROR) {
			/* send data */
			HAL_UART_Transmit(&hrxtx, (uint8_t *) (sens1.data->wptr - MSPARAM_PIXEL),
			MSPARAM_PIXEL * 2,
			MSPARAM_PIXEL * 2 * 100);
		}
	} else { /* DATA_FORMAT_ASCII */
		if (errcode != ERRC_NO_ERROR) {
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
 * Map the sensor status to the appropriate error code.
 */
static uint32_t map_status2errcode(uint8_t status) {
	switch (status) {
	case SENS_MEASURE_DONE:
		return ERRC_NO_ERROR;
	case SENS_ERR_TIMEOUT:
		return ERRC_TIMEOUT;
	case SENS_ERR_NO_EOS:
		return ERRC_NO_EOS;
	case SENS_ERR_EOS_EARLY:
		return ERRC_EOS_EARLY;
	default:
		return ERRC_UNKNOWN;
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

/*
 * check and parse for an command and set the
 * variable usrcmd if any command was found.
 *
 * The command have to be in the beginning
 * of the given buffer.
 *
 * @param buffer the buffer to check
 * @param size unused
 *
 * todo (future release) check the whole buffer up to size
 */
static void parse_extcmd(uint8_t *buffer, uint16_t size) {
	UNUSED(size);
	char *str, *alias;
	uint16_t sz, aliassz;

	/* init cmd structure */
	extcmd.cmd = USR_CMD_UNKNOWN;
	memset(&extcmd.arg_buffer, 0, ARGBUFFSZ);

	str = "measure\r";
	alias = "m\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_SINGLE_MEASURE_START;
		return;
	}

	str = "stream\r";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_STREAM_START;
		return;
	}

	str = "end\r";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_STREAM_END;
		return;
	}

	str = "getdata\r";
	alias = "gd\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_GET_DATA;
		return;
	}

	str = "#DEBUG#\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_DEBUG;
		return;
	}

	str = "rtc?\r";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_GET_RTC_TIME;
		return;
	}

	str = "ival?\r";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_GET_INTERVAL;
		return;
	}

	str = "itime?\r";
	alias = "i?\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_READ_ITIME;
		return;
	}

	str = "itime=";
	alias = "i=";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_WRITE_ITIME;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "format=";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_SET_FORMAT;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "rtc=";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_SET_RTC_TIME;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "ival=";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_SET_INTERVAL;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}
}

static void periodic_alarm_handler(void) {
	RTC_AlarmTypeDef a;
	uint16_t errc = 0;
	char ts_buff[32];
	debug("Periodic alarm \n");

	/* Set the alarm to new time according to the interval value.*/
	HAL_RTC_GetAlarm(&hrtc, &a, RTC_ALARM_A, RTC_FORMAT_BIN);
	rtc_set_alarmA_by_offset(&a.AlarmTime, &rtc_ival);

	/* Generate timestamp*/
	rtc_get_now_str(ts_buff, 32);

	/* Make a measurement */
	sensor_init();
	sensor_measure();
	last_sensor_status = sens1.status;
	errc = map_status2errcode(last_sensor_status);
	sensor_deinit();

#if HAS_SD
	int8_t res = 0;
	uint8_t err = 0;
	FIL *f = &SDFile;
	/* Store the measurement on SD */
	res = sd_mount();
	debug("mount: %u\n", res);
	res = sd_find_right_filename(fname_curr_postfix, &fname_curr_postfix, fname_buf, fname_buf_sz);
	debug("find: %u\n", res);
	res = sd_open_file_neworappend(f, fname_buf);
	debug("open: %u\n", res);

	if (res == FR_DISK_ERR) {
		/* Try to reinitialize driver. This can happen
		 * if Sd card was unplugged.*/
		FATFS_UnLinkDriver(SDPath);
		MX_FATFS_Init();
		/* try again..*/
		res = sd_open_file_neworappend(f, fname_buf);
		debug("relink+open: %u\n", res);
		if (res != FR_OK) {
			/* Some serios SD problems */
			res = sd_umount();
			return;
		}
	}
	/* Write metadata to SD: timestamp, errorcode, intergartion time */
	f_printf(f, "%S, %U, %LU, [", ts_buff, errc, sens1.itime);

	/* Write data */
	if (!err) {
		/* Lopp through measurement results and store to file */
		uint16_t *p = (uint16_t *) (sens1.data->wptr - MSPARAM_PIXEL);
		for (uint16_t i = 0; i < MSPARAM_PIXEL; ++i) {
			f_printf(f, "%U,", *(p++));
		}
	}
	f_printf(f, "]\n");
	res = f_close(f);
	debug("close: %u\n", res);
	res = sd_umount();
#endif

	/* Print what we wrote to sd.
	 * Use printf() instead of debug() to prevent 'dbg:' string before every value.
	 * If debug is disabled we don't do anything.*/
	if (tx_dbgflg) {
		printf("If SD: write to File: %s, data:\n", fname_buf);
		printf("%s, %u, %lu, [", ts_buff, errc, sens1.itime);
		uint16_t *p = (uint16_t *) (sens1.data->wptr - MSPARAM_PIXEL);
		for (uint16_t i = 0; i < MSPARAM_PIXEL; ++i) {
			printf("%u,", *(p++));
		}
		printf("]\n");
	}
}
