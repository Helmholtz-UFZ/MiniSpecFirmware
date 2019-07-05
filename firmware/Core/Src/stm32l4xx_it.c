/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32l4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <lib_spectrometer.h>
#include <lib_uart.h>
#include "main.h"
#include "stm32l4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "power.h"
#include "lib_cpu.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
 
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#include "usart.h"
#include "global_config.h"
#include "main_usr.h"

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern RTC_HandleTypeDef hrtc;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim5;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern UART_HandleTypeDef huart1;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */ 
/******************************************************************************/
/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
#define SysTick_Handler__OK
  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32L4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32l4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line2 interrupt.
  */
void EXTI2_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI2_IRQn 0 */

	/*
	 *
	 * DATA READY TO READ - ADC1 BUSY
	 * -------------------------------
	 *
	 * With every rising edge on the ADC-Busy-Line we come here and read the last value
	 * of the ADC conversion. The value is read from the 16 GPIO pins, which are directly
	 * connected to the parallel port of the ADC.
	 *
	 * This IR is only enabled [1] and disabled [2] in IRQHandlers.
	 *
	 * [1] TIM1_CC_IRQHandler()
	 * [2] TIM1_CC_IRQHandler() or TIM5_IRQHandler().
	 *
	 * TODO (future release) use DMA instead of manually save values.
	 */

	uint_fast16_t value0, value1;

	// -------------- nomore code here !! --------------
	if( __HAL_GPIO_EXTI_GET_IT(EXTADC_BUSY_Pin) != RESET )
	{
		__HAL_GPIO_EXTI_CLEAR_IT( EXTADC_BUSY_Pin );

		if( sens1.data->wptr < (sens1.data->base + sens1.data->words) )
		{
			// read ADC parallel-port-value
			value0 = GPIOA->IDR;
			value1 = GPIOC->IDR;
			*(sens1.data->wptr++) = (value1 << 16) | value0;
		}
	}
	// -------------- nomore code here !! --------------

#ifndef DO_NOT_USE_HAL_IRQ_HANDLER
#error this_will_fail
  /* USER CODE END EXTI2_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
  /* USER CODE BEGIN EXTI2_IRQn 1 */
#endif  /* DO_NOT_USE_HAL_IRQ_HANDLER */
#define EXTI2_IRQHandler__OK
  /* USER CODE END EXTI2_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel5_IRQn 0 */

  /* USER CODE END DMA1_Channel5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_rx);
  /* USER CODE BEGIN DMA1_Channel5_IRQn 1 */

  /* USER CODE END DMA1_Channel5_IRQn 1 */
}

/**
  * @brief This function handles TIM1 capture compare interrupt.
  */
void TIM1_CC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_CC_IRQn 0 */

	/*
	 * CAPTURE DATA END - END OF SCAN (EOS)
	 * ---------------------------------------
	 *
	 * Here we handle the capturing of the EOS of the sensor. EOS signal is send after
	 * all pixel-data (samples) are send by the sensor. If we catch the signal we dis-
	 * able the IR for the ADC-Busy-Line and return to the main program [1], which is
	 * waiting for the status to change.
	 *
	 * [1] see sensor_wait_for_measurement_done() in micro_spec.c
	 *
	 */

	if( (TIM1->SR & TIM_SR_CC2IF) && (TIM1->DIER & TIM_DIER_CC2IE) )
	{
		// clear IR flag
		__HAL_TIM_CLEAR_IT( &htim1, TIM_IT_CC2 );

		// Disable IR for ADC-busy-line. We also disable EOS,
		// as we may got it early and we don't want to catch
		// an other or many others.
		NVIC_ClearPendingIRQ( EXTI2_IRQn );
		NVIC_DisableIRQ( EXTI2_IRQn );

		// do we got the eos to early ?
		// this may happen if EOS is crosstalked by
		// other signals. shielding (GND) it well.
		if( TIM1->CCR2 < TRG_TO_EOS - 1 )
		{
			sens1.status = SENS_ERR_EOS_EARLY;
		}
		else
		{
			sens1.status = SENS_EOS_CAPTURED;
		}
		__HAL_TIM_DISABLE_IT( &htim1, TIM_IT_CC2 );
	}

	/*
	 * CAPTURE DATA START
	 * -------------------
	 *
	 * If we counted MSPARAM_CAPTURE_PXL_ST many TRG edges we start capturing the
	 * values from the ADC. Here we enable the IR for the ADC-Busy-Line. To examine
	 * the start of capturing, the TEST signal is set high at the same time
	 * (automatically by the TIM1). One can read TEST on the oscilloscope.
	 */
	if( (TIM1->SR & TIM_SR_CC4IF) && (TIM1->DIER & TIM_DIER_CC4IE) )
	{
		// clear IR flag
		__HAL_TIM_CLEAR_IT( &htim1, TIM_IT_CC4 );
//		TIM1->SR &= ~TIM_SR_CC4IF;

		// Enable IR for ADC-busy-line.
		__HAL_GPIO_EXTI_CLEAR_IT( EXTADC_BUSY_Pin );
		NVIC_ClearPendingIRQ( EXTI2_IRQn );
		NVIC_EnableIRQ( EXTI2_IRQn );

		sens1.status = SENS_CAPTURE_DATA;
	}

#ifndef DO_NOT_USE_HAL_IRQ_HANDLER
#error this_will_fail
  /* USER CODE END TIM1_CC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_CC_IRQn 1 */
#endif  /* DO_NOT_USE_HAL_IRQ_HANDLER */
#define TIM1_CC_IRQHandler__OK
  /* USER CODE END TIM1_CC_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */
	/*
	 * Here all u(s)art3 related IRs are handled by the HAL.
	 *
	 * We only add a case for handling of capturing a predefined sign ('\r' aka. carriage-return).
	 * If we capture such a sign we assume the user had entered a complete command and now want us
	 * to handle it appropriate, as we'll do :). We set some parameter here and handle it in the
	 * main-loop.
	 */

	// catch carriage return as 'end of cmd'-flag
	if( ((RXTX->ISR & USART_ISR_CMF) != RESET) && ((RXTX->CR1 & USART_CR1_CMIE) != RESET) )
	{
		__HAL_UART_CLEAR_IT( &hrxtx, USART_ISR_CMF );
		__HAL_UART_DISABLE_IT( &hrxtx, UART_IT_CM );
		rxtx.cmd_bytes = hrxtx.RxXferSize - hrxtx.hdmarx->Instance->CNDTR;
		rxtx.wakeup = true;
	}

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */
  if(rxtx.wakeup || hrxtx.ErrorCode){
		leave_LPM_from_ISR();
  }
#define RXTX_IRQHandler__OK
  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles EXTI line[15:10] interrupts.
  */
void EXTI15_10_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */

  /* USER CODE END EXTI15_10_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */

  /* USER CODE END EXTI15_10_IRQn 1 */
}

/**
  * @brief This function handles RTC alarm interrupt through EXTI line 18.
  */
void RTC_Alarm_IRQHandler(void)
{
  /* USER CODE BEGIN RTC_Alarm_IRQn 0 */

  /* USER CODE END RTC_Alarm_IRQn 0 */
  HAL_RTC_AlarmIRQHandler(&hrtc);
  /* USER CODE BEGIN RTC_Alarm_IRQn 1 */

  /* USER CODE END RTC_Alarm_IRQn 1 */
}

/**
  * @brief This function handles TIM5 global interrupt.
  */
void TIM5_IRQHandler(void)
{
  /* USER CODE BEGIN TIM5_IRQn 0 */

  /* USER CODE END TIM5_IRQn 0 */
  HAL_TIM_IRQHandler(&htim5);
  /* USER CODE BEGIN TIM5_IRQn 1 */

	// Disable IR for ADC-busy-line...
	NVIC_DisableIRQ( EXTI2_IRQn );

	// ..and the EOS capturing.
	__HAL_TIM_DISABLE_IT( &htim1, TIM_IT_CC2 );

	sens1.status = SENS_ERR_TIMEOUT;

#define TIM5_IRQHandler__OK
  /* USER CODE END TIM5_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/*
 * Some security checks, as CubeMX may delete IRQHandler when
 * they are not needed. This checks are added to easy find this
 * kinds of errors as they are hard to track and can happen easily.
 */
#if (!defined EXTI2_IRQHandler__OK \
	|| !defined TIM1_CC_IRQHandler__OK \
	|| !defined RXTX_IRQHandler__OK \
	|| !defined SysTick_Handler__OK \
	|| !defined TIM5_IRQHandler__OK)
#error "IRQ_Handler missing. May it was deleted by CubeMX ?"
#endif
/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
