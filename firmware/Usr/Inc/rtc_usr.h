/*
 * rtc_usr.h
 *
 *  Created on: Mar 6, 2019
 *      Author: palmb_ubu
 */

#ifndef INC_RTC_USR_H_
#define INC_RTC_USR_H_

#include "rtc.h"

extern RTC_TimeTypeDef rtc_ival;

uint8_t rtc_parse_datetime(char* str, RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate);
uint8_t rtc_parse_interval(char *str, RTC_TimeTypeDef *sTime);

#endif /* INC_RTC_USR_H_ */