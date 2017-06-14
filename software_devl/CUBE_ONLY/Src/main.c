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
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config( void );
void Error_Handler( void );
static void MX_NVIC_Init( void );

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
volatile uint8_t cmd_flag;
volatile uint16_t cmd_bytes;
/* USER CODE END 0 */

int main( void )
{

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration----------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_TIM2_Init();
	MX_TIM1_Init();
	MX_TIM5_Init();
	MX_USART3_UART_Init();

	/* Initialize interrupts */
	MX_NVIC_Init();

	/* USER CODE BEGIN 2 */

	//timer

	// ST
	TIM2->CCER |= TIM_CCER_CC3E;

	// TEST
	TIM1->CCER |= TIM_CCER_CC4E;
	__HAL_TIM_MOE_ENABLE( &htim1 );

	//start tim2 (ST)
	__HAL_TIM_ENABLE( &htim2 );

//	//modbus activation
//	uint32_t cr1 = 0;
//	char c;
//	//save cr1 status + reset RE and UE bit for modifying ADD[7:0]
//	cr1 = USART3->CR1;
//	USART3->CR1 &= ~(USART_CR1_RE | USART_CR1_UE);
//
//	c = '\r';
//	USART3->CR2 = (USART_CR2_ADD_Msk & (c << USART_CR2_ADD_Pos));
//
//	// restore RE and UE bit
//	USART3->CR1 = cr1;
//	//enable char match IR
//	USART3->CR1 |= USART_CR1_CMIE;
//
//	static uint8_t recv_buf[51];
//	memset( recv_buf, 0, 50 );
//	recv_buf[50] = '\0';
//
//	static uint8_t send_buf[51];
//	memset( send_buf, 'a', 50 );
//	send_buf[50] = '\0';
//	HAL_UART_Receive_IT( &huart3, recv_buf, 50 );
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while( 1 )
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		/* TIMER TEST */
//		TIM_CCxChannelCmd((&htim2)->Instance, TIM_CHANNEL_3, TIM_CCx_ENABLE);
//		HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_2); // HACK as PWM_START do not start
// TIM2 CH3 high @ first rising edge (first full pulse) -->> pulse 1
// low @ 10th rising edge (9 full pulses before) -->> period 9
// with low also update event (ARR overflow)
//		HAL_Delay( 50 );
		/* UART TEST*/

//		if( cmd_flag != 0 )
//		{
//			cmd_flag = 0;
//
//			if( memchr( (const void *) recv_buf, 'S', cmd_bytes ) != NULL )
//			{
//
////				HAL_UART_Transmit( &huart3, (uint8_t *) "\n", 1, 100 );
//				HAL_UART_Transmit( &huart3, (uint8_t *) send_buf, sizeof(send_buf), 8000000 );
////				HAL_UART_Transmit( &huart3, (uint8_t *) "\n", 1, 100 );
//				HAL_Delay( 50 );
//			}
//			memset( recv_buf, 0, cmd_bytes );
//			cmd_bytes = 0;
//			HAL_UART_Receive_IT( &huart3, recv_buf, 50 );
//		}
	}
	/* USER CODE END 3 */

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

/** NVIC Configuration
 */
static void MX_NVIC_Init( void )
{
	/* EXTI2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority( EXTI2_IRQn, 0, 0 );
	HAL_NVIC_EnableIRQ( EXTI2_IRQn );
	/* USART3_IRQn interrupt configuration */
	HAL_NVIC_SetPriority( USART3_IRQn, 0, 0 );
	HAL_NVIC_EnableIRQ( USART3_IRQn );
	/* TIM1_UP_TIM16_IRQn interrupt configuration */
	HAL_NVIC_SetPriority( TIM1_UP_TIM16_IRQn, 0, 0 );
	HAL_NVIC_EnableIRQ( TIM1_UP_TIM16_IRQn );
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void Error_Handler( void )
{
	/* USER CODE BEGIN Error_Handler */
	/* User can add his own implementation to report the HAL error return state */
	while( 1 )
	{
	}
	/* USER CODE END Error_Handler */
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */

}

#endif

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
