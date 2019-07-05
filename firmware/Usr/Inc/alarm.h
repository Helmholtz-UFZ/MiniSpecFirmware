/*
 * alarm.h
 *
 *  Created on: Jul 5, 2019
 *      Author: palmb_ubu
 */

#ifndef INC_ALARM_H_
#define INC_ALARM_H_

#include "main_usr.h"


RTC_TimeTypeDef get_closest_next_alarm(runtime_config_t *rc);
void set_initial_alarm(runtime_config_t *rc);

#endif /* INC_ALARM_H_ */
