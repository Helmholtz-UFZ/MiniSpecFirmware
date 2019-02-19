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

/**
 * error code enumeration
 */
typedef enum
{
	ERRC_NO_ERROR = 0,
	ERRC_UNKNOWN,
	ERRC_NO_EOS,
	ERRC_EOS_EARLY,
	ERRC_TIMEOUT
} error_code;

/**
 * enum for the defined commands
 */
typedef enum usr_cmd_enum
{
	USR_CMD_UNKNOWN,
	USR_CMD_SET_FORMAT, // 0 - raw/bin or 1 - ascii
	USR_CMD_SET_SENSOR, // UNUSED (for future use) - choose if sens 1 or sens 2
	USR_CMD_WRITE_ITIME,
	USR_CMD_READ_ITIME,
	USR_CMD_GET_DATA, // UNUSED (for future use) save data and return on request
	USR_CMD_SINGLE_MEASURE_START,
	USR_CMD_STREAM_START,
	USR_CMD_STREAM_END,
	USR_CMD_DEBUG,
} usr_cmd_enum_t;


int main_usr( void );
void cpu_enter_sleep_mode( void );
void cpu_enter_run_mode( void );

#endif /* __MAIN_USR_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/