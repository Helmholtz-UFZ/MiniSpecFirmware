/*
 * rtc_usr.h
 *
 *  Created on: Mar 6, 2019
 *      Author: palmb_ubu
 */

#ifndef INC_RTC_USR_H_
#define INC_RTC_USR_H_

#include "rtc.h"

extern volatile bool rtc_alarmA_occured;

uint8_t rtc_parse_datetime(char* str, RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate);
uint8_t rtc_parse_interval(char *str, RTC_TimeTypeDef *sTime);
uint8_t rtc_set_alarmA_by_offset(RTC_TimeTypeDef *time, RTC_TimeTypeDef *ival);
void rtc_get_now_str(char *buffer, uint32_t sz);
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc);

#endif /* INC_RTC_USR_H_ */
