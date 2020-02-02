/*
 * sysrc.h
 *
 *  Created on: Feb 2, 2020
 *      Author: Bert Palm
 */

#ifndef INC_SYSRC_H_
#define INC_SYSRC_H_

#include "rtc.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * the data format specifies the datatype of the
 * arguments and the data to/from external system.
 **/
typedef enum {
	DATA_FORMAT_BIN = 0,
	DATA_FORMAT_ASCII = 1,

} dataformat_t;


typedef enum
{
	MODE_OFF = 0,
	MODE_ENDLESS = 1,
	MODE_STARTEND = 2,
	MODE_TRIGGERED = 3,
} run_mode_t;

#define RCCONF_MAX_ITIMES  32
#define RCCONF_MAX_ITERATIONS  32
typedef struct
{
	uint8_t iterations;
	int32_t itime[RCCONF_MAX_ITIMES];
	uint8_t itime_index;

	uint32_t aa_lower;
	uint32_t aa_upper;

	// see also ival_mode_t
	run_mode_t mode;

	RTC_TimeTypeDef start;
	RTC_TimeTypeDef end;
	RTC_TimeTypeDef ival;
	RTC_TimeTypeDef next_alarm;

	/* verbosity of debug messages */
	uint8_t debuglevel;

	// the format to use for communication can be: DATA_FORMAT_BIN or DATA_FORMAT_ASCII
	bool format;
	bool sleeping;

} runtime_config_t;


extern runtime_config_t rc;

void init_mode(runtime_config_t *rc);

#endif /* INC_SYSRC_H_ */
