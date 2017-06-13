/**
 ******************************************************************************
 * File Name          : main.c
 * Description        : Main program body
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
#include "main.h"
#include "stm32l4xx_hal.h"
#include "tim.h"
#include "gpio.h"
#include "global_include.h"
#include "micro_spec.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"

/* Private variables ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config( void );
void Error_Handler( void );
static void MX_NVIC_Init( void );

int main( void )
{

	/* MCU Configuration----------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	// HAL inits.
	// Mark all functions (!) that differ from the CUBE_ONLY project.
	MX_GPIO_Init(); 	// modified
	MX_TIM1_Init();		// todo modified
	MX_TIM2_Init();	// todo modified
	MX_TIM5_Init();	// todo modified
	MX_USART3_UART_Init();

	// our own inits
	USART3_Init();

	/* Initialize interrupts */
	MX_NVIC_Init(); 	//modified

//	volatile uint32_t cnt0=0, cnt1=0, cnt2=0;

//	__HAL_TIM_SET_AUTORELOAD( &htim5, 0xFFFFFFFF );

//	__HAL_TIM_ENABLE( &htim5 );
//	cnt0 = TIM5->CNT;
//	__NOP();__NOP();
//	cnt1 = TIM5->CNT;
//	cnt2 = TIM5->CNT;

//	__NOP();
//
//	NVIC_EnableIRQ( SENS_EOS_IRQn );
//	NVIC_EnableIRQ( EXTADC1_BUSY_IRQn );
//	NVIC_EnableIRQ( TIM2_IRQn );
//	HAL_NVIC_SetPriority(SENS_EOS_IRQn, 0, 0);
//	HAL_NVIC_SetPriority(EXTADC1_BUSY_IRQn, 5, 0);
//	HAL_NVIC_SetPriority(TIM2_IRQn, 10, 0);

	micro_spec_init();
	micro_spec_set_integration_time( 100000 );

	HAL_UART_Receive_IT( &huart3, uart3_recv_buffer.base, uart3_recv_buffer.size );

	while( 1 )
	{

		// usr pushed enter
		if( uart3_cmd_received )
		{
			uart3_cmd_received = RESET;

			// usr pushed 'S'
			if( memchr( uart3_recv_buffer.base, 'S', uart3_cmd_bytes ) != NULL )
			{
				// MEASURE
				micro_spec_measure_init();
				micro_spec_measure_start();
				micro_spec_measure_deinit();
				HAL_UART_Transmit( &huart3, (uint8_t *) sens1_buffer.buf, sens1_buffer.size, 1000 );
			}
			memset( uart3_recv_buffer.base, 0, MIN( uart3_cmd_bytes, uart3_recv_buffer.size ) );
			uart3_cmd_bytes = RESET;
			HAL_UART_Receive_IT( &huart3, uart3_recv_buffer.base, uart3_recv_buffer.size );
		}

		HAL_Delay( 10 );
	}
}

/** NVIC Configuration
 */
static void MX_NVIC_Init( void )
{
	HAL_NVIC_EnableIRQ( TIM2_IRQn_TRG );
	HAL_NVIC_EnableIRQ( USART3_IRQn );

// EXTI9_5_IRQn_EOS:	en/dis in ISR
// EXTI2_IRQn_BUSY1:	 en/dis in ISR
}

/** System Clock Configuration
 */
void SystemClock_Config( void )
{

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInit;

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = 16;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 10;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if( HAL_RCC_OscConfig( &RCC_OscInitStruct ) != HAL_OK )
	{
		Error_Handler();
	}

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if( HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_4 ) != HAL_OK )
	{
		Error_Handler();
	}

	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
	PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
	if( HAL_RCCEx_PeriphCLKConfig( &PeriphClkInit ) != HAL_OK )
	{
		Error_Handler();
	}

	HAL_RCC_MCOConfig( RCC_MCO1, RCC_MCO1SOURCE_HSI, RCC_MCODIV_16 );

	/**Configure the main internal regulator output voltage
	 */
	if( HAL_PWREx_ControlVoltageScaling( PWR_REGULATOR_VOLTAGE_SCALE1 ) != HAL_OK )
	{
		Error_Handler();
	}

	/**Configure the Systick interrupt time
	 */
	HAL_SYSTICK_Config( HAL_RCC_GetHCLKFreq() / 1000 );

	/**Configure the Systick
	 */
	HAL_SYSTICK_CLKSourceConfig( SYSTICK_CLKSOURCE_HCLK );

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority( SysTick_IRQn, 0, 0 );
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void Error_Handler( void )
{
	/* User can add his own implementation to report the HAL error return state */
	while( 1 )
	{
	}
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
