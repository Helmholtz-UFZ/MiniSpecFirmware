/*
 * power.h
 *
 *  Created on: Jun 16, 2019
 *      Author: rg
 */

#ifndef INC_POWER_H_
#define INC_POWER_H_

#include "global_config.h"

void cpu_sleep(void);
void cpu_stop0(void);
void cpu_stop1(void);
void cpu_stop2(void);
void cpu_standby(void);
void power_switch_EN(bool);
void leave_LPM_from_ISR(void);
void cpu_enter_LPM(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif /* INC_POWER_H_ */
