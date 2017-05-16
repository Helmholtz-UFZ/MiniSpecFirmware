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
	MX_GPIO_Init();
	MX_TIM2_Init();

	/* Initialize interrupts */
	MX_NVIC_Init();

	micro_spec_init();
	micro_spec_set_integration_time( 100 );

	while( 1 )
	{
		micro_spec_measure_init();
		micro_spec_measure_start();
		micro_spec_measure_deinit();

		HAL_Delay(100);
	}

}

/** System Clock Configuration
 */
void SystemClock_Config( void )
{

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

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

/** NVIC Configuration
 */
static void MX_NVIC_Init( void )
{
	/* EXTI15_10_IRQn interrupt configuration */
	HAL_NVIC_SetPriority( EXTI15_10_IRQn, 0, 0 );
	/* TIM2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority( TIM2_IRQn, 0, 0 );
	/* EXTI9_5_IRQn interrupt configuration */
	HAL_NVIC_SetPriority( EXTI9_5_IRQn, 0, 0 );
	/* EXTI2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority( EXTI2_IRQn, 0, 0 );
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
