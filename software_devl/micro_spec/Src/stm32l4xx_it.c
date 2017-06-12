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
 * @brief This function handles TIM2 global interrupt.
 *
 * TRG
 *
 * This is called when we catch the MSPARAM_UNUSED_TRG_CNTth TRG-pulse. We
 * enable the IR for the EXTADC_BUSY pin and the EOS.*/
void TIM2_IRQHandler( void )
{
//	volatile uint16_t value0, value1; //todo kill volatile
//	volatile uint32_t value = 0;
//	TIM_HandleTypeDef *htim = &htim2;
//	HAL_TIM_IRQHandler( htim );

//	value0 = GPIOA->IDR & SENS1_PA_mask;
//	value1 = GPIOC->IDR & SENS1_PC_mask;
//	value = (value1 << 8) | value0;

//	if(__HAL_TIM_GET_FLAG(htim, TIM_FLAG_CC3)){
//		if(__HAL_TIM_GET_IT_SOURCE(htim, TIM_IT_CC3)){
//
//		}
//	}
	if( (TIM2->SR & TIM_SR_CC3IF) && (TIM2->DIER & TIM_DIER_CC3IE) )
	{
		// clear IR flag
		TIM2->SR &= ~TIM_SR_CC3IF;
		status = MS_TIM2_CC;
		__HAL_GPIO_EXTI_CLEAR_IT( EXTADC1_BUSY_Pin );
		__HAL_GPIO_EXTI_CLEAR_IT( SENS_EOS_Pin );
		NVIC_EnableIRQ( EXTADC1_BUSY_IRQn );
		NVIC_EnableIRQ( SENS_EOS_IRQn );
		__HAL_TIM_SET_AUTORELOAD( &htim5, 0xFFFFFFFF );
		__HAL_TIM_ENABLE( &htim5 );
	}

	if( (TIM2->SR & TIM_SR_UIF) && (TIM2->DIER & TIM_DIER_UIE) )
	{
		// clear IR flag
		TIM2->SR &= ~TIM_SR_UIF;

		NVIC_DisableIRQ( EXTADC1_BUSY_IRQn );
		NVIC_DisableIRQ( SENS_EOS_IRQn );
		__HAL_TIM_DISABLE_IT( &htim2, TIM_IT_UPDATE );

		status = MS_TIM2_DONE;
	}
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
//	cnt0 = TIM5->CNT;
	uint8_t value0, value1;

//	HAL_GPIO_EXTI_IRQHandler()
	if( __HAL_GPIO_EXTI_GET_IT(EXTADC1_BUSY_Pin) != RESET )
	{
		__HAL_GPIO_EXTI_CLEAR_IT( EXTADC1_BUSY_Pin );

		if( sens1_buffer.w_idx < BUFFER_SIZE )
		{
//			status = MS_READ_DATA;
			// read ADC parallel-port-value
			value0 = GPIOA->IDR;
			value1 = GPIOC->IDR;
//			sens1_buffer.w_idx++;
			sens1_buffer.buf[sens1_buffer.w_idx++] = (value1 << 8) | value0;
		}
//		else // hack dont work with this uncommented -- WHY?? race condidion ?
//		{
//			status = MS_BUFFER_FULL;
//		}
	}
//	cnt1 = TIM5->CNT;
//	__NOP();
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
void EXTI9_5_IRQHandler( void )
{
	volatile uint32_t tmp;
	tmp = TIM2->CNT;
//	v1 = TIM2->CNT;
//	v2 = TIM2->CNT;
//	v3 = TIM2->CNT;
	if( __HAL_GPIO_EXTI_GET_IT(SENS_EOS_Pin) != RESET )
	{
		__HAL_GPIO_EXTI_CLEAR_IT( SENS_EOS_Pin );
		eos_trg_count = tmp;

//	if( EXTI->PR1 & SENS_EOS_Pin )
//	{
		// clear pending IR
//		EXTI->PR1 |= SENS_EOS_Pin;
//		NVIC_DisableIRQ( EXTADC1_BUSY_IRQn );
//		__HAL_TIM_DISABLE_IT( &htim2, TIM_IT_UPDATE );
//		NVIC_DisableIRQ( SENS_EOS_IRQn );

//		if( v2 == v1 && v2 == v3 )
//		{
//		}
//		else
//		{
//			v1;
//		}

//		status = MS_EOS;
	}
}

/**
 * @brief This function handles USART3 global interrupt.
 */
void USART3_IRQHandler( void )
{
	HAL_UART_IRQHandler( &huart3 );
}

