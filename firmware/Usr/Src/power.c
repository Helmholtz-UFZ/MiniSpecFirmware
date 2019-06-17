/*
 * power.c
 *
 *  Created on: Jun 16, 2019
 *      Author: rg
 */
#include "power.h"
#include "main.h"
#include "main_usr.h"
#include "rtc_usr.h"
#include "stdio.h"
#include "dma.h"
#include "sdmmc.h"
#include "tim.h"
#include "usart.h"
#include "fatfs.h"
#include "gpio.h"
#include "usart_usr.h"
#include "tim_usr.h"

bool asleep = false;

/* Borrowed from main.c */
extern void SystemClock_Config(void);

static void sys_reinit(void);
static void sys_deinit(void);

static void sys_reinit(void){
	SystemClock_Config();
	MX_GPIO_Init();
	HAL_UART_Init(&hrxtx);
	rxtx_init();
	rxtx.debug = true;
	NVIC_EnableIRQ(RXTX_IRQn);
	rxtx_restart_listening();
	__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM);                         //
	HAL_Delay(100);
}

static void sys_deinit(void){
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Pin = GPIO_PIN_All;

	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

	// keep the CMDS_EN Line active
	GPIO_InitStruct.Pin &= ~(CMDS_EN_Pin);
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void cpu_sleep(void) {
	debug("enter sleep mode\n");
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWR_EnterSLEEPMode( PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	HAL_ResumeTick();
	debug("leave sleep mode\n");
}

void cpu_stop0(void){
	debug("enter stop0 mode\n");
	sys_deinit();
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnterSTOP0Mode( PWR_STOPENTRY_WFI);
	HAL_ResumeTick();
	sys_reinit();
	debug("leave stop0 mode\n");
}


void cpu_stop1(void){
	debug("enter stop1 mode\n");
	sys_deinit();
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnterSTOP1Mode( PWR_STOPENTRY_WFI);
	HAL_ResumeTick();
	sys_reinit();
	debug("leave stop1 mode\n");
}

void cpu_stop2(void){
	debug("enter stop2 mode\n");
	sys_deinit();
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
	HAL_ResumeTick();
	sys_reinit();
	debug("leave stop2 mode\n");
}

void cpu_standby(void){
	debug("enter standby mode\n");
	sys_deinit();
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnableSRAM2ContentRetention();
	HAL_PWR_EnterSTANDBYMode();
	HAL_ResumeTick();
	sys_reinit();
	debug("leave standby mode\n");
}

/**
 * @brief cpu_awake()
 * EXECUTED BY ISR - KEEP IT SHORT
 */
void leave_LPM_from_ISR(void) {
	// wake up after handling the actual IR
	CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPONEXIT_Msk));
}

void power_switch_EN(bool on){
#if USE_POWER_SWITCH
	if(on){
		HAL_GPIO_WritePin(POWER5V_SWITCH_ENBL_GPIO_Port, POWER5V_SWITCH_ENBL_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(POWER5V_SWITCH_ENBL_GPIO_Port, POWER5V_SWITCH_ENBL_Pin, GPIO_PIN_RESET);
	}
#else
	UNUSED(on);
#endif
}

void cpu_enter_LPM(void){
	asleep = true;
	if( HAL_GPIO_ReadPin(CMDS_EN_GPIO_Port, CMDS_EN_Pin) == GPIO_PIN_SET){
		cpu_sleep();
	}else{
		cpu_stop2();
	}
	HAL_Delay(100);
	asleep = false;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == CMDS_EN_Pin){
		if(asleep){
			leave_LPM_from_ISR();
		}
	}
}
