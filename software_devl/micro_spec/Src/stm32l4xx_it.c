/**
 ******************************************************************************
 * @brief   Interrupt Service Routines.
 * @file    stm32l4xx_it.c
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
#include "micro_spec.h"
#include "global_include.h"
#include "buffer.h"
#include "tim.h"
#include "usart.h"

/* External variables --------------------------------------------------------*/

extern volatile bool uart3_cmd_received;
extern volatile uint16_t uart3_cmd_bytes;

/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */
/******************************************************************************/

volatile uint16_t eos_trg_count;

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler( void )
{
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
}

/******************************************************************************/
/* STM32L4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32l4xx.s).                    */
/******************************************************************************/

/**
 * @brief This function handles TIM1 capture compare interrupt.
 *
 * TEST
 *
 * CCRx
 *
 * We enable the IR for (ADC1_BUSY) the ADC-trigger.
 *
 */
void TIM1_CC_IRQHandler( void )
{
	// channel 4 compare IR
	if( (TIM1->SR & TIM_SR_CC4IF) && (TIM1->DIER & TIM_DIER_CC4IE) )
	{
		// clear IR flag
		TIM1->SR &= ~TIM_SR_CC4IF;
		status = MS_TIM1_CC;
		__HAL_GPIO_EXTI_CLEAR_IT( EXTADC1_BUSY_Pin );
		NVIC_EnableIRQ( EXTI2_IRQn_BUSY1 );
	}
}

/**
 * @brief This function handles TIM1 update interrupt
 *
 * TRG
 *
 * This is called when we catch the MSPARAM_CAPTURE_PXL_ENDth TRG-pulse. We
 * disable everything as we are done.*/
void TIM1_UP_TIM16_IRQHandler( void )
{
	// clear IR flag
	TIM1->SR &= ~TIM_SR_UIF;

	NVIC_DisableIRQ( EXTI2_IRQn_BUSY1 );

	status = MS_TIM1_DONE;
}

/**
 * @brief This function handles EXTI line2 interrupt.
 *
 * ADC1_BUSY
 *
 * With every falling edge we can read the current data value of the
 * (external) ADC conversion on 16 GPIO pins (self-defined parallel port)
 *
 * This IR is enabled by the TRG IR and disabled by the EOS IR.
 *
 * TODO use DMA instead of manually save values
 */
volatile uint32_t cnt0 = 0, cnt1 = 0, cnt2 = 0;
void EXTI2_IRQHandler( void )
{
	uint8_t value0, value1;

	// -------------- nomore code here !! --------------
	if( __HAL_GPIO_EXTI_GET_IT(EXTADC1_BUSY_Pin) != RESET )
	{
		__HAL_GPIO_EXTI_CLEAR_IT( EXTADC1_BUSY_Pin );

		if( sens1_buffer.w_idx < BUFFER_MAX_IDX )
		{
			// read ADC parallel-port-value
			value0 = GPIOA->IDR;
			value1 = GPIOC->IDR;
			sens1_buffer.buf[sens1_buffer.w_idx++] = (value1 << 8) | value0;
		}
	}
	// -------------- nomore code here !! --------------
}

/**
 * @brief This function handles External Line[9:5] Interrupts.
 *
 * EOS
 *
 * When every sample/pixel was sended by the sensor, it generates the EOS signal.
 *
 * We disable the ADC1_BUSY IR, the EOS IR (this) and the TRG IR. We also read
 * the current TRG count.
 *
 */

/**
 * @brief This function handles USART3 global interrupt.
 */
void USART3_IRQHandler( void )
{
	HAL_UART_IRQHandler( &huart3 );
	// catch carriage return as 'end of cmd'-flag
	if( ((USART3->ISR & USART_ISR_CMF) != RESET) && ((USART3->CR1 & USART_CR1_CMIE) != RESET) )
	{
		__HAL_UART_CLEAR_IT( &huart3, USART_ISR_CMF );
		uart3_cmd_bytes = huart3.RxXferSize - huart3.RxXferCount;
		uart3_cmd_received = true;
	}

}

