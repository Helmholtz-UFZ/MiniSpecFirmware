/*
 * sd.h
 *
 *  Created on: Jul 5, 2019
 *      Author: palmb_ubu
 */

#ifndef INC_SD_H_
#define INC_SD_H_


#include "global_config.h"
#include "main_usr.h"

#define NO_SD	100

uint8_t measurement_to_SD(char *timestamp_str);
void inform_SD_reset(void);
void inform_SD_rtc(char *oldtimestamp_str);
uint8_t write_config_to_SD(runtime_config_t *rc);
uint8_t read_config_from_SD(runtime_config_t *rc);


#endif /* INC_SD_H_ */