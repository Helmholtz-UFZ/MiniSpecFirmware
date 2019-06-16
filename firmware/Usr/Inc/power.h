/*
 * power.h
 *
 *  Created on: Jun 16, 2019
 *      Author: rg
 */

#ifndef INC_POWER_H_
#define INC_POWER_H_

#include "global_config.h"

void sleep(void);
void stop0(void);
void stop1(void);
void stop2(void);
void standby(void);
void power_switch_EN(bool);
void leave_LPM_from_ISR(void);

#endif /* INC_POWER_H_ */
