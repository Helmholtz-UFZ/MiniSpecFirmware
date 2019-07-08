/*
 * rtc_usr.h
 *
 *  Created on: Mar 6, 2019
 *      Author: palmb_ubu
 */

#ifndef INC_LIB_RTC_H_
#define INC_LIB_RTC_H_

#include "rtc.h"

typedef struct
{
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;
} rtc_timestamp_t;

typedef struct
{
	volatile bool alarmA_wakeup;
}rtc_t;

extern rtc_t rtc;

bool rtc_time_leq(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b);
bool rtc_time_eq(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b);
bool rtc_time_lt(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b);
RTC_TimeTypeDef rtc_time_add(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b);
uint32_t rtc_time2seconds(RTC_TimeTypeDef *t);
RTC_TimeTypeDef rtc_seconds2time(uint32_t s);
uint8_t rtc_parsecheck_datetime(char* str, RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate);
uint8_t rtc_parse_time(char *str, RTC_TimeTypeDef *sTime);
uint8_t rtc_set_alarmA(RTC_TimeTypeDef *time);
uint8_t rtc_set_alarmA_by_offset(RTC_TimeTypeDef *time, RTC_TimeTypeDef *ival);
void rtc_get_now_str(char *buffer, uint32_t sz);
rtc_timestamp_t rtc_get_now(void);
RTC_TimeTypeDef rtc_get_alermAtime(void);
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc);

#endif /* INC_LIB_RTC_H_ */