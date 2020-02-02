/*
 * wakeup.h
 *
 *  Created on: Feb 2, 2020
 *      Author: rg
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

#endif /* INC_WAKEUP_H_ */
