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

/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */
/******************************************************************************/

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
 * @brief This function handles TIM2 global interrupt.
 *
 * 89th TRG done
 *
 * This is called when we catch the 89th TRG-pulse. We enable the external ADC
 * and the corosponing IR and wait for TODO EXTADC1Busy to go high/low(?) */
void TIM2_IRQHandler( void )
{
	TIM_HandleTypeDef *htim = &htim2;
	HAL_TIM_IRQHandler( &htim2 );

// delay end, pulse start
//	if( __HAL_TIM_GET_FLAG( htim, TIM_FLAG_CC3 ) )
//	{
//		__HAL_TIM_CLEAR_IT( htim, TIM_IT_CC3 );
//		status = MS_COUNT_TRG;
//	}
//
//	// pulse end
//	if( __HAL_TIM_GET_FLAG(htim, TIM_FLAG_UPDATE) != RESET )
//	{
//		__HAL_TIM_CLEAR_IT( htim, TIM_IT_UPDATE );
//		status = MS_COUNT_TRG_DONE;
//	}
	status = MS_READ_ADC_DONE;
}

//void TIM1_UP_TIM16_IRQHandler( void )
//{
//	TIM_HandleTypeDef *htim = &htim1;
//	if( __HAL_TIM_GET_FLAG(htim, TIM_FLAG_UPDATE) != RESET )
//	{
//		__HAL_TIM_CLEAR_IT( htim, TIM_IT_UPDATE );
//		status = MS_ST_SIGNAL_TIM_DONE;
//	}
//}

/**
 * @brief This function handles EXTI line2 interrupt.
 *
 * ADC1_BUSY
 *
 * With every falling edge we can read curren value of the
 * (external) ADC conversion on 16 GPIO (self-defined parallel port)
 *
 * If we have read all pixels we disable this IR and continue in the
 * micro_spec_measure_start() function (micro_spec.c).
 */
void EXTI2_IRQHandler( void )
{
	uint16_t value0, value1;

	if( EXTI->PR1 & EXTADC1_BUSY_Pin )
	{
		// clear pending interrupt
		EXTI->PR1 |= EXTADC1_BUSY_Pin;

		status = MS_READ_ADC;

		// +1 as the first value is not valid
		if( sens1_buffer.w_idx < MSPARAM_PIXEL + 1 )
		{
			//TODO use DMA [EXTI2_IRQHandler()]
			// read ADC parallel-port-value
			value0 = GPIOA->IDR & SENS1_PA_mask;
			value1 = GPIOC->IDR & SENS1_PC_mask;
			sens1_buffer.buf[sens1_buffer.w_idx++] = (value1 << 8) | value0;
		}
		else
		{
			// disable ADC IR (this) and disable the ADC itself
			NVIC_DisableIRQ( EXTADC1_BUSY_IRQn );
			HAL_GPIO_WritePin( EXTADC_EN_GPIO_Port, EXTADC_EN_Pin, GPIO_PIN_RESET );
			status = MS_READ_ADC_DONE;
		}
	}
}

/**
 * @brief This function handles EXTI line[15:10] interrupts.
 *
 * ADC2_BUSY
 *
 */
//void EXTI15_10_IRQHandler( void )
//{
//	HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_13 );
//
//}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
