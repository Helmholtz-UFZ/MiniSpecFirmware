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

/* Borrowed from main.c */
extern void SystemClock_Config(void);

static void sys_reinit(void);
static void sys_deinit(void);

static void sys_reinit(void){
	SystemClock_Config();

	MX_GPIO_Init();
	MX_DMA_Init();
	MX_TIM2_Init();
	MX_TIM1_Init();
	MX_TIM5_Init();
	MX_TIM3_Init();
	MX_SDMMC1_SD_Init();
	MX_FATFS_Init();
	MX_USART1_UART_Init();

	/* Initialize interrupts see MX_NVIC_Init(); in main.c */
	HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI2_IRQn);
	HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
	HAL_NVIC_SetPriority(TIM5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM5_IRQn);
	HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
	HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

	/* reinit statemachine hw modifications */
	usr_hw_init();
	// reanable character match
	__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM);
}

static void sys_deinit(void){
//	HAL_UART_Abort(&hrxtx);
	HAL_UART_DeInit(&hrxtx);
	HAL_TIM_Base_DeInit(&htim1);
	HAL_TIM_Base_DeInit(&htim2);
	HAL_TIM_Base_DeInit(&htim3);
	HAL_TIM_Base_DeInit(&htim5);
//	HAL_SD_Abort(&hsd1);
	HAL_SD_DeInit(&hsd1);
	FATFS_UnLinkDriver(SDPath);

	// all pins to analog (save a lot power
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;

	GPIO_InitStruct.Pin = GPIO_PIN_All;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

	/* keep the CMDS_EN Line active listening and
	 * the Powerswitch line tied to GND */
	GPIO_InitStruct.Pin &= ~(CMDS_EN_Pin);
	GPIO_InitStruct.Pin &= ~(POWER5V_SWITCH_ENBL_Pin);
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

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

	while (!rxtx.wakeup && !rtc.alarmA_wakeup) {
		HAL_Delay(200); // prevent very fast switching of CMD_EN Pin
		if( HAL_GPIO_ReadPin(CMDS_EN_GPIO_Port, CMDS_EN_Pin) == GPIO_PIN_SET){
			debug("enter light sleep mode\n");
			cpu_sleep();
		}else{
			debug("enter deep sleep mode\n");
			sys_deinit();
			while (!rxtx.wakeup && !rtc.alarmA_wakeup && HAL_GPIO_ReadPin(CMDS_EN_GPIO_Port, CMDS_EN_Pin) == GPIO_PIN_RESET){
				cpu_stop2();
			}
			sys_reinit();
		}
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == CMDS_EN_Pin){
		leave_LPM_from_ISR();
	}
}
