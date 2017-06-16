/**
 ******************************************************************************
 * File Name          : USART.c
 * Description        : This file provides code for the configuration
 *                      of the USART instances.
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
#include "usart.h"

#include "gpio.h"
#include "global_config.h"
#include "string.h"
#include "stdlib.h"

UART_HandleTypeDef huart3;

simple_buffer uart3_recv_buffer;

volatile bool uart3_cmd_received;
volatile uint16_t uart3_cmd_bytes;

usr_cmd_t usr_cmd = USR_CMD_UNKNOWN;

uint32_t usr_cmd_data = 0;

/* USART3 init function */

void MX_USART3_UART_Init( void )
{

	huart3.Instance = USART3;
	huart3.Init.BaudRate = 115200;
	huart3.Init.WordLength = UART_WORDLENGTH_8B;
	huart3.Init.StopBits = UART_STOPBITS_1;
	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT | UART_ADVFEATURE_DMADISABLEONERROR_INIT;
	huart3.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
	huart3.AdvancedInit.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;
	if( HAL_UART_Init( &huart3 ) != HAL_OK )
	{
		Error_Handler();
	}

}

void HAL_UART_MspInit( UART_HandleTypeDef* uartHandle )
{

	GPIO_InitTypeDef GPIO_InitStruct;
	if( uartHandle->Instance == USART3 )
	{
		/* USER CODE BEGIN USART3_MspInit 0 */

		/* USER CODE END USART3_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_USART3_CLK_ENABLE()
		;

		/**USART3 GPIO Configuration
		 PB11     ------> USART3_RX
		 PC10     ------> USART3_TX
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_11;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
		HAL_GPIO_Init( GPIOB, &GPIO_InitStruct );

		GPIO_InitStruct.Pin = GPIO_PIN_10;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
		HAL_GPIO_Init( GPIOC, &GPIO_InitStruct );

		/* USER CODE BEGIN USART3_MspInit 1 */

		/* USER CODE END USART3_MspInit 1 */
	}
}

void HAL_UART_MspDeInit( UART_HandleTypeDef* uartHandle )
{

	if( uartHandle->Instance == USART3 )
	{
		/* USER CODE BEGIN USART3_MspDeInit 0 */

		/* USER CODE END USART3_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_USART3_CLK_DISABLE();

		/**USART3 GPIO Configuration
		 PB11     ------> USART3_RX
		 PC10     ------> USART3_TX
		 */
		HAL_GPIO_DeInit( GPIOB, GPIO_PIN_11 );

		HAL_GPIO_DeInit( GPIOC, GPIO_PIN_10 );

		/* Peripheral interrupt Deinit*/
		HAL_NVIC_DisableIRQ( USART3_IRQn );

	}
}

void usart3_init( void )
{
	/* Cmd activation
	 * We enable the automatic character recognition for the carriage return (CR) char.*/

	uint32_t cr1 = 0;
	char trigger_char;
	//save cr1 status + reset RE and UE bit for modifying ADD[7:0]
	cr1 = USART3->CR1;
	USART3->CR1 &= ~(USART_CR1_RE | USART_CR1_UE);

	trigger_char = '\r';
	USART3->CR2 |= (USART_CR2_ADD_Msk & (trigger_char << USART_CR2_ADD_Pos));

	// restore RE and UE bit
	USART3->CR1 = cr1;
	//enable char match IR-Flag
	USART3->CR1 |= USART_CR1_CMIE;

	uart3_cmd_received = 0;
	uart3_cmd_bytes = 0;

	/* set up buffer */

	// memory allocation
	static uint8_t recv_mem_block[SERIAL_RX_BUF_SZ];
	memset( recv_mem_block, 0, SERIAL_RX_BUF_SZ );

	uart3_recv_buffer.size = SERIAL_RX_BUF_SZ;
	uart3_recv_buffer.base = recv_mem_block;
}

void usart3_receive_handler( void )
{
	usr_cmd = USR_CMD_UNKNOWN;

	// usr pushed enter
	if( uart3_cmd_received )
	{
		uart3_cmd_received = 0;

		// usr pressed 'S'
		if( memcmp( uart3_recv_buffer.base, "start single", uart3_cmd_bytes ) == 0 )
		{
			usr_cmd = USR_CMD_SINGLE_MEASURE_START;
		}

		else if( memcmp( uart3_recv_buffer.base, "start continuous", uart3_cmd_bytes ) == 0 )
		{
			usr_cmd = USR_CMD_CONTINUOUS_MEASURE_START;
		}

		else if( memcmp( uart3_recv_buffer.base, "end continuous", uart3_cmd_bytes ) == 0 )
		{
			usr_cmd = USR_CMD_CONTINUOUS_MEASURE_END;
		}

		else if( memcmp( uart3_recv_buffer.base, "write_int_time=\"", 16 ) == 0 )
		{
			// find first "
			char *num_st = memchr( uart3_recv_buffer.base, '\"', 16 );
			// find second ", max value: 4 294 967 295 = 10 chars
			char *num_end = memchr( num_st, '\"', 10 );

			if( num_end == NULL )
			{
				goto l_invalid;
			}

			//check for unwanted chars
			if( strcspn( num_st, "0123456789" ) != 0 )
			{
				goto l_invalid;
			}

			// set the value
			usr_cmd_data = strtol( num_st, NULL, 10 );
			usr_cmd = USR_CMD_WRITE_INTEGRATION_TIME;
		}

		else if( memcmp( uart3_recv_buffer.base, "read_int_time", uart3_cmd_bytes ) == 0 )
		{
			usr_cmd = USR_CMD_READ_INTEGRATION_TIME;
		}

		// if something goes wrong we come here
		l_invalid:

		// restart listening
		memset( uart3_recv_buffer.base, 0, MIN( uart3_cmd_bytes, uart3_recv_buffer.size ) );
		uart3_cmd_bytes = 0;
		HAL_UART_AbortReceive_IT( &huart3 );
		HAL_UART_Receive_IT( &huart3, uart3_recv_buffer.base, uart3_recv_buffer.size );
	}
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
