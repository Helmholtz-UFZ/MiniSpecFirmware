/*
 * alarm.h
 *
 *  Created on: Jul 5, 2019
 *      Author: Bert Palm
 */

#ifndef INC_ALARM_H_
#define INC_ALARM_H_

#include <mainloop.h>


void init_mode(runtime_config_t *rc);
void init_timetype(RTC_TimeTypeDef *time);

#endif /* INC_ALARM_H_ */
