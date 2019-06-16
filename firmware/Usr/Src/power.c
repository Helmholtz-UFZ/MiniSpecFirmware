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


extern void SystemClock_Config(void);

void reinit(void){
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

	// from MX_NVIC_Init()
	HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
	HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 0);
	HAL_NVIC_SetPriority(TIM5_IRQn, 0, 0);

	// from main_usr.c, init()
	rxtx_init();
	tim1_Init();
	tim2_Init();
	tim5_Init();

	// from main_usr.c, main_usr()
	NVIC_EnableIRQ(RXTX_IRQn);
	__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM);
}

void deinit(void){
	__HAL_RCC_DMA1_CLK_DISABLE();
	HAL_TIM_Base_DeInit(&htim1);
	HAL_TIM_Base_DeInit(&htim2);
	HAL_TIM_Base_DeInit(&htim3);
	HAL_TIM_Base_DeInit(&htim5);
	HAL_SD_DeInit(&hsd1);
	HAL_UART_DeInit(&hrxtx);

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = (uint16_t) 0xFFFE;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_All;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOD_CLK_DISABLE();
}

void sleep(void) {
	debug("sleep\n");
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWR_EnterSLEEPMode( PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	HAL_ResumeTick();
}

void stop0(void){
	debug("stop0\n");
	deinit();
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnterSTOP0Mode( PWR_STOPENTRY_WFI);
	HAL_ResumeTick();
	reinit();
}


void stop1(void){
	debug("stop1\n");
	deinit();
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnterSTOP1Mode( PWR_STOPENTRY_WFI);
	HAL_ResumeTick();
	reinit();
}

void stop2(void){
	debug("stop2\n");
	deinit();
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
	HAL_ResumeTick();
	reinit();
}

void standby(void){
	debug("standby\n");
	deinit();
	HAL_SuspendTick();
	HAL_PWR_EnableSleepOnExit();
	HAL_PWREx_EnableSRAM2ContentRetention();
	HAL_PWR_EnterSTANDBYMode();
	HAL_ResumeTick();
	reinit();
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

