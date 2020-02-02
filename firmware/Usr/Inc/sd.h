/*
 * sd.h
 *
 *  Created on: Jul 5, 2019
 *      Author: Bert Palm
 */

#ifndef INC_SD_H_
#define INC_SD_H_


#include <globalconfig.h>
#include <mainloop.h>
#include "sysrc.h"

#define FNAME_BUF_SZ 	128
typedef struct
{
	char buf[FNAME_BUF_SZ];
	uint16_t postfix;
} filename_t;

#define NO_SD	100

uint8_t measurement_to_SD(char *timestamp_str);
void inform_SD_reset(void);
void inform_SD_rtc(char *oldtimestamp_str);
uint8_t write_config_to_SD(runtime_config_t *rc);
uint8_t read_config_from_SD(runtime_config_t *rc);


#endif /* INC_SD_H_ */
