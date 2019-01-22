/**
  ******************************************************************************
  * @file    stm32l4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l4xx.h"
#include "stm32l4xx_it.h"

/* USER CODE BEGIN 0 */

#include "usart.h"
#include "global_include.h"
#include "micro_spec.h"

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim5;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern UART_HandleTypeDef huart1;

/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System service call via SWI instruction.
*/
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
* @brief This function handles Pendable request for system service.
*/
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
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

	uint8_t value0, value1;

	// -------------- nomore code here !! --------------
	if( __HAL_GPIO_EXTI_GET_IT(EXTADC_BUSY_Pin) != RESET )
	{
		__HAL_GPIO_EXTI_CLEAR_IT( EXTADC_BUSY_Pin );

		if( sens1.data->wptr < (sens1.data->base + sens1.data->words) )
		{
			// read ADC parallel-port-value
			value0 = GPIOA->IDR;
			value1 = GPIOC->IDR;
			*(sens1.data->wptr++) = (value1 << 8) | value0;
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

		rxtx_cmd_bytes = hrxtx.RxXferSize - hrxtx.hdmarx->Instance->CNDTR;
		rxtx_CR_recvd = true;


		cpu_enter_run_mode();
	}

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

#define USART1_IRQHandler__OK
  /* USER CODE END USART1_IRQn 1 */
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
	|| !defined USART1_IRQHandler__OK \
	|| !defined SysTick_Handler__OK \
	|| !defined TIM5_IRQHandler__OK)
#warning "IRQ_Handler missing. May it was deleted by CubeMX ?"
#endif
/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
