/*
 * alarm.h
 *
 *  Created on: Jul 5, 2019
 *      Author: Bert Palm
 */

#ifndef INC_ALARM_H_
#define INC_ALARM_H_

#include <mainloop.h>
#include <globalconfig.h>
#include "rtc.h"


RTC_TimeTypeDef get_closest_next_alarm(runtime_config_t *rc);
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc);

#endif /* INC_ALARM_H_ */
