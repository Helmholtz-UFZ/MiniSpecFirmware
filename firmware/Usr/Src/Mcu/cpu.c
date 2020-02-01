/*
 * lib_cpu.c
 *
 *  Created on: Jul 5, 2019
 *      Author: Bert Palm
 */

#include "main.h"

void cpu_sleep(void) {
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWR_EnterSLEEPMode( PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	HAL_ResumeTick();
}

void cpu_stop0(void){
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnterSTOP0Mode( PWR_STOPENTRY_WFI);
	HAL_ResumeTick();
	HAL_Delay(1);
}


void cpu_stop1(void){
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnterSTOP1Mode( PWR_STOPENTRY_WFI);
	HAL_ResumeTick();
	HAL_Delay(1); // avoid spurios BOR. see ERRATA 2.3.21
}

void cpu_stop2(void){
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
	HAL_ResumeTick();
	HAL_Delay(1); // avoid spurios BOR. see ERRATA 2.3.21
}

void cpu_standby(void){
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnableSRAM2ContentRetention();
	HAL_PWR_EnterSTANDBYMode();
	HAL_ResumeTick();
}

/**
 * @brief cpu_awake()
 * EXECUTED BY ISR - KEEP IT SHORT
 */
void leave_LPM_from_ISR(void) {
	// wake up after handling the actual IR
	CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPONEXIT_Msk));
}

