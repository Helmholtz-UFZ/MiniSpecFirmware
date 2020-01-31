/*
 * lib_cpu.h
 *
 *  Created on: Jul 5, 2019
 *      Author: Bert Palm
 */

#ifndef INC_LIB_CPU_H_
#define INC_LIB_CPU_H_

void cpu_sleep(void);
void cpu_stop0(void);
void cpu_stop1(void);
void cpu_stop2(void);
void cpu_standby(void);
void leave_LPM_from_ISR(void);

#endif /* INC_LIB_CPU_H_ */
