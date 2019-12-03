/*
 * power.c
 *
 *  Created on: Jun 16, 2019
 *      Author: rg
 */
#include <lib_rtc.h>
#include <lib_timer.h>
#include <lib_uart.h>
#include "power.h"
#include "main.h"
#include "main_usr.h"
#include "lib_cpu.h"
#include "stdio.h"
#include "dma.h"
#include "sdmmc.h"
#include "tim.h"
#include "usart.h"
#include "fatfs.h"
#include "gpio.h"

/* Borrowed from main.c */
extern void SystemClock_Config(void);

static void sys_deinit(uint8_t mode);
static void sys_reinit(uint8_t mode);

static void sys_reinit(uint8_t mode) {
	if (mode == DEEP_SLEEP_MODE) {
		SystemClock_Config();
	}

	MX_GPIO_Init();
	MX_SDMMC1_SD_Init();
	MX_FATFS_Init();

	MX_TIM2_Init();
	MX_TIM1_Init();
	MX_TIM5_Init();
	MX_TIM3_Init();
	tim1_Init();
	tim2_Init();
	tim5_Init();

	if (mode == DEEP_SLEEP_MODE) {
		MX_DMA_Init();
		MX_USART1_UART_Init();

		/* Initialize interrupts see MX_NVIC_Init(); in main.c */
		HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
		HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
		HAL_NVIC_EnableIRQ(USART1_IRQn);
		HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

		/* reinit statemachine hw modifications */
		usr_hw_init();
		// reanable character match
		__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM);
	}
}

/**
 * deinit system for power saving.
 *
 * if mode is DEEP_SLEEP_MODE (0) only the POWERSWITCH and CMDS_EN pins are left functional.
 * if mode is LIGHT_SLEEP_MODE (1) the POWERSWITCH, the CMDS_EN and uart (txrx) pins
 * are left functional.
 */
static void sys_deinit(uint8_t mode) {
	if (mode == DEEP_SLEEP_MODE) {
		HAL_UART_DeInit(&hrxtx);
	}

	HAL_TIM_Base_DeInit(&htim1);
	HAL_TIM_Base_DeInit(&htim2);
	HAL_TIM_Base_DeInit(&htim3);
	HAL_TIM_Base_DeInit(&htim5);
	HAL_SD_DeInit(&hsd1);
	FATFS_UnLinkDriver(SDPath);

	// all pins to analog (save a lot power
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;

	GPIO_InitStruct.Pin = GPIO_PIN_All;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

	/* Keep Depug functionality */
	if (mode == LIGHT_SLEEP_MODE) {
		GPIO_InitStruct.Pin = GPIO_PIN_All;
		GPIO_InitStruct.Pin &= ~(GPIO_PIN_13 | GPIO_PIN_14);
	}
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* keep the CMDS_EN Line active listening and
	 * the Powerswitch line tied to GND */
	GPIO_InitStruct.Pin = GPIO_PIN_All;
	GPIO_InitStruct.Pin &= ~(CMDS_EN_Pin);
	GPIO_InitStruct.Pin &= ~(POWER5V_SWITCH_ENBL_Pin);

	// keep uart alive
	if (mode == LIGHT_SLEEP_MODE) {
		GPIO_InitStruct.Pin &= ~(GPIO_PIN_6 | GPIO_PIN_7);
	}
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * Enables the Power-Switch. The 5V-Domain and
 * the 3V domain are powered on if on_off is true,
 * otherwise the domains are powered off.
 */
void power_switch_EN(bool on_off) {
#if USE_POWER_SWITCH
	GPIO_PinState state;
	state = HAL_GPIO_ReadPin(POWER5V_SWITCH_ENBL_GPIO_Port, POWER5V_SWITCH_ENBL_Pin);
	if (state == GPIO_PIN_RESET && on_off) {

		// enable the tim channels for the ST- and EOS- signal so the line is pulled down
		// by the uC. This ensures a low signal on these lines.
		TIM2->CCER |= TIM_CCER_CC3E;
		TIM1->CCER |= TIM_CCER_CC4E;
		HAL_GPIO_WritePin(POWER5V_SWITCH_ENBL_GPIO_Port, POWER5V_SWITCH_ENBL_Pin, GPIO_PIN_SET);

		// wait for stabilisation of voltage reference buffer
		HAL_Delay(VOLTAGEREF_STABILIZATION_DELAY);

	} else if (state == GPIO_PIN_SET && !on_off) {

		HAL_GPIO_WritePin(POWER5V_SWITCH_ENBL_GPIO_Port, POWER5V_SWITCH_ENBL_Pin, GPIO_PIN_RESET);
	}
#else
	UNUSED(on_off);
#endif
}

void init_rtc_batterie_backup(void){

}

void cpu_enter_LPM(void) {

	bool asleep = false;

	while (!rxtx.wakeup && !rtc.alarmA_wakeup) {
		if (asleep){
			HAL_Delay(200); // prevent very fast switching of CMD_EN Pin
		}
		asleep = true;
		if (HAL_GPIO_ReadPin(CMDS_EN_GPIO_Port, CMDS_EN_Pin) == GPIO_PIN_SET) {
			debug("enter light sleep mode\n");
			sys_deinit(LIGHT_SLEEP_MODE);
			while (!rxtx.wakeup && !rtc.alarmA_wakeup && HAL_GPIO_ReadPin(CMDS_EN_GPIO_Port, CMDS_EN_Pin) == GPIO_PIN_SET) {
				cpu_sleep();
			}
			sys_reinit(LIGHT_SLEEP_MODE);
		} else {
			debug("enter deep sleep mode\n");
			sys_deinit(DEEP_SLEEP_MODE);
			while (!rxtx.wakeup && !rtc.alarmA_wakeup && HAL_GPIO_ReadPin(CMDS_EN_GPIO_Port, CMDS_EN_Pin) == GPIO_PIN_RESET) {
				cpu_stop2();
			}
			sys_reinit(DEEP_SLEEP_MODE);
		}
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == CMDS_EN_Pin) {
		leave_LPM_from_ISR();
	}
}
