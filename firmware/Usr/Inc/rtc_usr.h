/*
 * rtc_usr.h
 *
 *  Created on: Mar 6, 2019
 *      Author: palmb_ubu
 */

#ifndef INC_RTC_USR_H_
#define INC_RTC_USR_H_

#include "rtc.h"

typedef struct
{
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time;
} rtc_timestamp_t;

typedef struct
{
	volatile bool alarmA_wakeup;
}rtc_t;

extern rtc_t rtc;

void rtc_init(void);
uint8_t rtc_parse_datetime(char* str, RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate);
uint8_t rtc_parse_time(char *str, RTC_TimeTypeDef *sTime);
uint8_t rtc_parse_interval(char *str, RTC_TimeTypeDef *sTime);
uint8_t rtc_set_alarmA_by_offset(RTC_TimeTypeDef *time, RTC_TimeTypeDef *ival);
uint8_t rtc_update_alarmA(RTC_TimeTypeDef *time);
void rtc_get_now_str(char *buffer, uint32_t sz);
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc);

#endif /* INC_RTC_USR_H_ */
