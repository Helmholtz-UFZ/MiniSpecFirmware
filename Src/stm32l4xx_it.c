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
 * TRG
 *
 * This is called when we catch the MSPARAM_UNUSED_TRG_CNTth TRG-pulse. We
 * enable the IR for the EXTADC_BUSY pin and the EOS.*/
void TIM2_IRQHandler( void )
{
//	volatile uint16_t value0, value1; //todo kill volatile
//	volatile uint32_t value = 0;
//	TIM_HandleTypeDef *htim = &htim2;
//	HAL_TIM_IRQHandler( &htim2 );

//	value0 = GPIOA->IDR & SENS1_PA_mask;
//	value1 = GPIOC->IDR & SENS1_PC_mask;
//	value = (value1 << 8) | value0;

	if( TIM2->SR & TIM_SR_CC3IF )
	{
		// clear IR flag
		TIM2->SR &= ~TIM_SR_CC3IF;
		status = MS_COUNT_TRG;
		__HAL_GPIO_EXTI_CLEAR_IT(EXTADC1_BUSY_Pin);
		__HAL_GPIO_EXTI_CLEAR_IT(SENS_EOS_Pin);
		NVIC_EnableIRQ( EXTADC1_BUSY_IRQn );
		NVIC_EnableIRQ( SENS_EOS_IRQn );
	}

	if( TIM2->SR & TIM_SR_UIF )
	{
		// clear IR flag
		TIM2->SR &= ~TIM_SR_UIF;
		status = MS_FAIL;
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
void EXTI2_IRQHandler( void )
{
	uint16_t value0, value1;

	if( EXTI->PR1 & EXTADC1_BUSY_Pin )
	{
		// clear pending interrupt
		EXTI->PR1 |= EXTADC1_BUSY_Pin;

		status = MS_READ_ADC;

		if( sens1_buffer.w_idx < sens1_buffer.size )
		{
			// read ADC parallel-port-value
			value0 = GPIOA->IDR & SENS1_PA_mask;
			value1 = GPIOC->IDR & SENS1_PC_mask;
			sens1_buffer.buf[sens1_buffer.w_idx++] = (value1 << 8) | value0;
		}
		else
		{
			status = MS_FAIL;
		}
	}
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
	volatile uint16_t trg_count;
	uint32_t v1,v2,v3;
	if( EXTI->PR1 & SENS_EOS_Pin )
	{
		// clear pending IR
		EXTI->PR1 |= SENS_EOS_Pin;
		NVIC_DisableIRQ( EXTADC1_BUSY_IRQn );
//		NVIC_DisableIRQ( TIM2_IRQn );
		NVIC_DisableIRQ( SENS_EOS_IRQn );
		v1 = TIM2->CNT;
		v2 = TIM2->CNT;
		v3 = TIM2->CNT;

		if(v2 == v1){}

		status = MS_READ_ADC_DONE;
	}
}

