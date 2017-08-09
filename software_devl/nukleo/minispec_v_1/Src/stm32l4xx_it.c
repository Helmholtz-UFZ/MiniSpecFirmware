/**
  ******************************************************************************
  * @file    stm32l4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 STMicroelectronics
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

extern volatile bool uart3_cmd_received;
extern volatile uint16_t uart3_cmd_bytes;
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim1;
extern UART_HandleTypeDef huart3;

/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */
#define SysTick_Handler__OK
  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
  /* USER CODE BEGIN SysTick_IRQn 1 */

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
#ifndef DO_NOT_USE_HAL_IRQ_HANDLER
#error this_will_fail
  /* USER CODE END EXTI2_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
  /* USER CODE BEGIN EXTI2_IRQn 1 */
#endif  /* DO_NOT_USE_HAL_IRQ_HANDLER */
#define EXTI2_IRQHandler__OK

	/**
	 *
	 * ADC1_BUSY
	 *
	 * With every IR we got a falling edge on the ADC-Busy-Line we can read the value
	 * of the the current ADC conversion. The value is read from the 16 GPIO pins
	 * connected to the parallel port of the ADC.
	 *
	 * This IR is enabled by the TIM1_CC_IRQHandler() (counter reaches TIM1_CCR4) and
	 * disabled by the TIM1_UP_TIM16_IRQHandler() (counter reaches TIM1_ARR).
	 *
	 * TODO use DMA instead of manually save values
	 */

	uint8_t value0, value1;

	// -------------- nomore code here !! --------------
	if( __HAL_GPIO_EXTI_GET_IT(EXTADC1_BUSY_Pin) != RESET )
	{
		__HAL_GPIO_EXTI_CLEAR_IT( EXTADC1_BUSY_Pin );

		if( hms1.data->wptr < (hms1.data->base + hms1.data->words) )
		{
			// read ADC parallel-port-value
			value0 = GPIOA->IDR;
			value1 = GPIOC->IDR;
			// todo pointer instead ?
			*(hms1.data->wptr++) = (value1 << 8) | value0;
		}
	}
	// -------------- nomore code here !! --------------

  /* USER CODE END EXTI2_IRQn 1 */
}

/**
* @brief This function handles TIM1 update interrupt and TIM16 global interrupt.
*/
void TIM1_UP_TIM16_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_TIM16_IRQn 0 */
#ifndef DO_NOT_USE_HAL_IRQ_HANDLER
#error this_will_fail
  /* USER CODE END TIM1_UP_TIM16_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_TIM16_IRQn 1 */
#endif  /* DO_NOT_USE_HAL_IRQ_HANDLER */
#define TIM1_UP_TIM16_IRQHandler__OK
	/*
	 * We get this IR when the TIM1 counter reaches the value, which is written in the ARR.
	 * This means we got todo ( TIM1_ARR - TIM1_CCR4 )+-1 ?? many ADC-conversions and are
	 * done now. We disable this IR and set the status to 'done', so we can continue in the
	 * main program [1] , where we wait for this flag to be set.
	 *
	 * With this IR the TEST-signal automatically is set low.
	 *
	 *
	 *
	 * [1] micro_spec_wait_for_measurement_done() in microspec.c
	 */

	// clear IR flag
	TIM1->SR &= ~TIM_SR_UIF;

	// Disable IR for ADC-busy-line.
	NVIC_DisableIRQ( EXTI2_IRQn );

	hms1.status = MS_MEASUREMENT_ONGOING_TIM1_UP;
  /* USER CODE END TIM1_UP_TIM16_IRQn 1 */
}

/**
* @brief This function handles TIM1 capture compare interrupt.
*/
void TIM1_CC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_CC_IRQn 0 */
#ifndef DO_NOT_USE_HAL_IRQ_HANDLER
#error this_will_fail
  /* USER CODE END TIM1_CC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_CC_IRQn 1 */
#endif  /* DO_NOT_USE_HAL_IRQ_HANDLER */
#define TIM1_CC_IRQHandler__OK


	/*
	 * This handles the capture compare of the TIM1. If this IR occur we enable the
	 * IR on the ADC-Busy-Line. So every time when this line change the level from
	 * high to low we get an IR (EXTI2_IR) an read the value from the ADC. For easy
	 * examination/debugging one can read the TEST signal on the oscilloscope it is
	 * set high automatically with this IR and is reset automatically with the
	 * TIM1_UP_TIM16_IR (when the TIM1 counter reaches the ARR value). It should be
	 * at least remain high until the EOS occur.
	 */

	// channel 4 compare IR
	if( (TIM1->SR & TIM_SR_CC4IF) && (TIM1->DIER & TIM_DIER_CC4IE) )
	{
		// clear IR flag
		TIM1->SR &= ~TIM_SR_CC4IF;
		hms1.status = MS_MEASUREMENT_ONGOING_TIM1_CC;
		__HAL_GPIO_EXTI_CLEAR_IT( EXTADC1_BUSY_Pin );

		// Enable IR for ADC-busy-line.
		NVIC_EnableIRQ( EXTI2_IRQn );
	}
  /* USER CODE END TIM1_CC_IRQn 1 */
}

/**
* @brief This function handles USART3 global interrupt.
*/
void USART3_IRQHandler(void)
{
  /* USER CODE BEGIN USART3_IRQn 0 */
#define USART3_IRQHandler__OK
	/*
	 * Here all u(s)art3 related IRs are handled by the HAL.
	 *
	 * We only add a case for handling of capturing a predefined sign ('\r' aka. carriage-return).
	 * If we capture such a sign we assume the user had entered a complete command and now want us
	 * to handle it appropriate, as we'll do :). We set some parameter here and handle it in the
	 * main-loop.
	 */

  /* USER CODE END USART3_IRQn 0 */
  HAL_UART_IRQHandler(&huart3);
  /* USER CODE BEGIN USART3_IRQn 1 */

	// catch carriage return as 'end of cmd'-flag
	if( ((USART3->ISR & USART_ISR_CMF) != RESET) && ((USART3->CR1 & USART_CR1_CMIE) != RESET) )
	{
		__HAL_UART_CLEAR_IT( &huart3, USART_ISR_CMF );
		uart3_cmd_bytes = huart3.RxXferSize - huart3.RxXferCount;
		uart3_cmd_received = true;
	}
  /* USER CODE END USART3_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/*
 * Some security checks, as CubeMX may delete IRQHandler when
 * they are not needed. This checks are added to easy find this
 * kinds of errors as they are hard to track and can happen easily.
 */

#ifdef DO_NOT_USE_HAL_IRQ_HANDLER
void dummy_function_to_ignore_warnings()
{
	UNUSED( htim1 );
}
#endif /*DO_NOT_USE_HAL_IRQ_HANDLER*/
#if (!defined EXTI2_IRQHandler__OK \
	|| !defined TIM1_UP_TIM16_IRQHandler__OK \
	|| !defined TIM1_CC_IRQHandler__OK \
	|| !defined USART3_IRQHandler__OK \
	|| !defined SysTick_Handler__OK )
#warning "IRQ_Handler missing. May it was deleted by CubeMX ?"
#endif
/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
