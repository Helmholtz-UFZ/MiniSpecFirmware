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
#include "fatfs.h"
#include "datetime.h"

#define FNAME_BUF_SZ 	128
typedef struct
{
	char buf[FNAME_BUF_SZ];
	uint16_t postfix;
} filename_t;

#define NO_SD	100

#ifdef USE_FORMAT_SD_CARD
FRESULT sd_format(void);
#endif

FRESULT sd_mount(void);
FRESULT sd_umount(void);

uint8_t sd_write_measurement(rtc_timestamp_t ts);
void sd_write_timechange_info(rtc_timestamp_t old);
void sd_write_reset_info(void);
uint8_t write_config_to_SD(runtime_config_t *rc);
uint8_t read_config_from_SD(runtime_config_t *rc);


#endif /* INC_SD_H_ */
