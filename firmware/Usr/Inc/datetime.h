/*
 * rtc_usr.h
 *
 *  Created on: Mar 6, 2019
 *      Author: Bert Palm
 */

#ifndef INC_DATETIME_H_
#define INC_DATETIME_H_

#include "rtc.h"
#include <stdbool.h>

#define TS_TO_PRINTCALL(x) "20%02u-%02u-%02uT%02u:%02u:%02u", x.date.Year, x.date.Month, x.date.Date, x.time.Hours, x.time.Minutes, x.time.Seconds


typedef struct
{
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;
} rtc_timestamp_t;


bool rtc_time_leq(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b);
bool rtc_time_eq(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b);
bool rtc_time_lt(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b);
RTC_TimeTypeDef rtc_time_add(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b);
uint32_t rtc_time2seconds(RTC_TimeTypeDef *t);
RTC_TimeTypeDef rtc_seconds2time(uint32_t s);
uint8_t rtc_parsecheck_datetime(char* str, RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate);
uint8_t rtc_parse_time(char *str, RTC_TimeTypeDef *sTime);
rtc_timestamp_t rtc_get_now(void);
void init_timetype(RTC_TimeTypeDef *time);

#endif /* INC_DATETIME_H_ */
