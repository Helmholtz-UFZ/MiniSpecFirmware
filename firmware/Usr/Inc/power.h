/*
 * power.h
 *
 *  Created on: Jun 16, 2019
 *      Author: rg
 */

#ifndef INC_POWER_H_
#define INC_POWER_H_

#include "global_config.h"

#define DEEP_SLEEP_MODE 	0
#define LIGHT_SLEEP_MODE 	1

/** in milli seconds*/
#define VOLTAGEREF_STABILIZATION_DELAY	(50)


void power_switch_EN(bool);
void cpu_enter_LPM(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif /* INC_POWER_H_ */
