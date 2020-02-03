/*
 * wakeup.h
 *
 *  Created on: Feb 2, 2020
 *      Author: Bert Palm
 */

#ifndef INC_WAKEUP_H_
#define INC_WAKEUP_H_

#include <stdbool.h>

typedef struct {

	volatile bool alarmA;
	volatile bool triggerPin;
	volatile bool cmd;

} wakeup_t;

extern wakeup_t wakeup;

void wakeup_alarm_handler(void);
void wakeup_pintrigger_handler(void);
void wakeup_cmd_handler(void);

#endif /* INC_WAKEUP_H_ */
