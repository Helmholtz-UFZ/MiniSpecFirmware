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
#include "main.h"
#include "global_config.h"
#include "rtc_usr.h"

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

typedef struct
{
	RTC_TimeTypeDef start;
	RTC_TimeTypeDef end;
	RTC_TimeTypeDef ival;
	RTC_TimeTypeDef next_alarm;
	bool startSet;
	bool endSet;
	bool ivalSet;
} time_config_t;


#define MCONF_MAX_ITIMES  32
#define MCONF_MAX_ITERATIONS  32
typedef struct
{
	uint8_t iterations;
	uint32_t itime[MCONF_MAX_ITIMES];
	uint8_t itime_index;
} measure_config_t;

typedef struct
{
	bool format;
	bool stream;
	bool toSD;

} statemachine_config_t;

#define FNAME_BUF_SZ 	128
typedef struct
{
	char buf[FNAME_BUF_SZ];
	uint16_t postfix;
} filename_t;

int main_usr( void );
void cpu_enter_sleep_mode( void );
void cpu_enter_run_mode( void );

#endif /* __MAIN_USR_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
