/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_USR_H__
#define __MAIN_USR_H__

/* Includes ------------------------------------------------------------------*/
#include <lib_rtc.h>
#include "main.h"
#include "global_config.h"

/** the data format specifies the datatype of the
 * arguments and the data to/from external system.*/
#define DATA_FORMAT_BIN		0

/** @sa DATA_FORMAT_BIN */
#define DATA_FORMAT_ASCII	1

/** header for data in ASCII format
 *  make data look nice on terminal */
#define HEADER_STR	"          0     1     2     3     4     5     6     7     8     9"

/** @sa DATA_FORMAT_BIN */
#define DELIMITER_STR   "-----------------------------------------------------------------"

typedef enum
{
	IVAL_OFF = 0,
	IVAL_ENDLESS = 1,
	IVAL_STARTEND = 2,
	TRIGGERED = 3,
}ival_mode_t;

#define RCCONF_MAX_ITIMES  32
#define RCCONF_MAX_ITERATIONS  32
typedef struct
{
	uint8_t iterations;
	uint32_t itime[RCCONF_MAX_ITIMES];
	uint8_t itime_index;

	// see also ival_mode_t
	ival_mode_t mode;

	RTC_TimeTypeDef start;
	RTC_TimeTypeDef end;
	RTC_TimeTypeDef ival;
	RTC_TimeTypeDef next_alarm;

	/* Flag for enabling /disabling the debug() function
	 * during runtime with a usr command, namely 'debug'.*/
	bool use_debugprints;

	// the format to use for communication can be: DATA_FORMAT_BIN or DATA_FORMAT_ASCII
	bool format;
	bool stream;
	volatile bool sleeping;
	volatile bool trigger;

} runtime_config_t;

extern runtime_config_t rc;

#define FNAME_BUF_SZ 	128
typedef struct
{
	char buf[FNAME_BUF_SZ];
	uint16_t postfix;
} filename_t;

void run_init(void);
void run(void);
void usr_hw_init(void);
void init_timetype(RTC_TimeTypeDef *time);

#endif /* __MAIN_USR_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
