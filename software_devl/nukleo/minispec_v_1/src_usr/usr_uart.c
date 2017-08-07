/*
 * usr_uart.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include "usart.h"
#include "string.h"
#include "global_include.h"

simple_buffer uart3_recv_buffer;
usr_cmd_enum_t usrcmd = USR_CMD_UNKNOWN;
uint32_t usr_cmd_data = 0;

volatile bool uart3_cmd_received;
volatile uint16_t uart3_cmd_bytes;

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
	static uint8_t recv_mem_block[EXTERNAL_COMMUNICATION_UART_BUFFER_SIZE];
	memset( recv_mem_block, 0, EXTERNAL_COMMUNICATION_UART_BUFFER_SIZE );

	uart3_recv_buffer.size = EXTERNAL_COMMUNICATION_UART_BUFFER_SIZE;
	uart3_recv_buffer.base = recv_mem_block;
}

void usart3_receive_handler( void )
{
	usrcmd = USR_CMD_UNKNOWN;

	// usr pushed enter
	if( uart3_cmd_received )
	{
		uart3_cmd_received = 0;

		// usr pressed 'S'
		if( memcmp( uart3_recv_buffer.base, "measure", 7 ) == 0 )
		{
			usrcmd = USR_CMD_SINGLE_MEASURE_START;
		}

		else if( memcmp( uart3_recv_buffer.base, "stream", 6 ) == 0 )
		{
			usrcmd = USR_CMD_CONTINUOUS_MEASURE_START;
		}

		else if( memcmp( uart3_recv_buffer.base, "endstream", 9 ) == 0 )
		{
			usrcmd = USR_CMD_CONTINUOUS_MEASURE_END;
		}

		else if( memcmp( uart3_recv_buffer.base, "itime=", 6 ) == 0 )
		{
			// ignore the pre-string than read as unsigned long int.
			sscanf( uart3_recv_buffer.base, "%*6c%lu%*c", &usr_cmd_data );
			usrcmd = USR_CMD_WRITE_INTEGRATION_TIME;
		}

		else if( memcmp( uart3_recv_buffer.base, "itime?", 6 ) == 0 )
		{
			usrcmd = USR_CMD_READ_INTEGRATION_TIME;
		}

		// restart listening
		uint16_t sz = huart3.RxXferSize - huart3.RxXferCount;
		HAL_UART_AbortReceive( &huart3 );
		memset( uart3_recv_buffer.base, 0, sz );
		uart3_cmd_bytes = 0;
		HAL_UART_Receive_IT( &huart3, uart3_recv_buffer.base, uart3_recv_buffer.size );
	}
}
