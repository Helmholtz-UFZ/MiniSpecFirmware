/*
 * alarm.h
 *
 *  Created on: Jul 5, 2019
 *      Author: Bert Palm
 */

#ifndef INC_ALARM_H_
#define INC_ALARM_H_

#include "rtc.h"
#include "sysrc.h"


RTC_TimeTypeDef get_closest_next_alarm(runtime_config_t *rc);
uint8_t rtc_set_alarmA(RTC_TimeTypeDef *time);
uint8_t rtc_set_alarmA_by_offset(RTC_TimeTypeDef *time, RTC_TimeTypeDef *ival);
RTC_TimeTypeDef rtc_get_alermAtime(void);

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc);

#endif /* INC_ALARM_H_ */
