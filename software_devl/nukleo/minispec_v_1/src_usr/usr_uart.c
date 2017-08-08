/*
 * usr_uart.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include "usart.h"
#include "global_include.h"
#include "string.h"
#include <stdarg.h>

usr_cmd_enum_t usrcmd = USR_CMD_UNKNOWN;
uint32_t usr_cmd_data = 0;

volatile bool uart3_cmd_received;
volatile uint16_t uart3_cmd_bytes;

uart_buffer_t uart3_rx_buffer;
uart_buffer_t uart3_tx_buffer;

static void uart_alloc_mem( void );
static void parse_cmd( void );

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
	uart_alloc_mem();
}

/**
 * allocate some memory for the uart buffer.
 */
static void uart_alloc_mem( void )
{

	// uart3
	static uint8_t rx_mem_block3[UART_DEFAULT_RX_BUFFER_SZ];
	memset( rx_mem_block3, 0, UART_DEFAULT_RX_BUFFER_SZ );
	uart3_rx_buffer.size = UART_DEFAULT_RX_BUFFER_SZ;
	uart3_rx_buffer.base = rx_mem_block3;

	static uint8_t tx_mem_block3[UART_DEFAULT_TX_BUFFER_SZ];
	memset( tx_mem_block3, 0, UART_DEFAULT_TX_BUFFER_SZ );
	uart3_tx_buffer.size = UART_DEFAULT_TX_BUFFER_SZ;
	uart3_tx_buffer.base = tx_mem_block3;
}

void usart3_receive_handler( void )
{
	uint16_t sz;
	usrcmd = USR_CMD_UNKNOWN;

	// usr pushed enter
	if( uart3_cmd_received )
	{
		uart3_cmd_received = 0;
		sz = huart3.RxXferSize - huart3.RxXferCount;

		if( sz == 0 )
		{
			return;
		}
		
		parse_cmd();

		// restart listening
		HAL_UART_AbortReceive( &huart3 );
		memset( uart3_rx_buffer.base, 0, sz );
		uart3_cmd_bytes = 0;
		HAL_UART_Receive_IT( &huart3, uart3_rx_buffer.base, uart3_rx_buffer.size );
	}
}

static void parse_cmd( void )
{
	char *str;
	uint16_t sz;

	str = "format=";
	sz = strlen( str );
	if( memcmp( uart3_rx_buffer.base, str, sz ) == 0 )
	{
		sscanf( (char*) (uart3_rx_buffer.base + sz), "%lu", &usr_cmd_data );
		usrcmd = USR_CMD_SET_DATA_FORMAT;
		return;
	}

	str = "measure\r";
	sz = strlen( str );
	if( memcmp( uart3_rx_buffer.base, str, sz ) == 0 )
	{
		usrcmd = USR_CMD_SINGLE_MEASURE_START;
		return;
	}

	str = "stream\r";
	sz = strlen( str );
	if( memcmp( uart3_rx_buffer.base, str, sz ) == 0 )
	{
		usrcmd = USR_CMD_CONTINUOUS_MEASURE_START;
		return;
	}

	str = "end\r";
	sz = strlen( str );
	if( memcmp( uart3_rx_buffer.base, str, sz ) == 0 )
	{
		usrcmd = USR_CMD_CONTINUOUS_MEASURE_END;
		return;
	}

	str = "itime=";
	sz = strlen( str );
	if( memcmp( uart3_rx_buffer.base, str, sz ) == 0 )
	{
		// ignore the pre-string than read as unsigned long int.
		sscanf( (char*) (uart3_rx_buffer.base + sz), "%lu", &usr_cmd_data );
		usrcmd = USR_CMD_WRITE_INTEGRATION_TIME;
		return;
	}

	str = "itime?\r";
	sz = strlen( str );
	if( memcmp( uart3_rx_buffer.base, str, sz ) == 0 )
	{
		usrcmd = USR_CMD_READ_INTEGRATION_TIME;
		return;
	}

	// short aliases
	str = "m\r";
	sz = strlen( str );
	if( memcmp( uart3_rx_buffer.base, str, sz ) == 0 )
	{
		usrcmd = USR_CMD_SINGLE_MEASURE_START;
		return;
	}

	str = "i=";
	sz = strlen( str );
	if( memcmp( uart3_rx_buffer.base, str, sz ) == 0 )
	{
		// ignore the pre-string than read as unsigned long int.
		sscanf( (char*) (uart3_rx_buffer.base + sz), "%lu", &usr_cmd_data );
		usrcmd = USR_CMD_WRITE_INTEGRATION_TIME;
		return;
	}

	str = "i?\r";
	sz = strlen( str );
	if( memcmp( uart3_rx_buffer.base, str, sz ) == 0 )
	{
		usrcmd = USR_CMD_READ_INTEGRATION_TIME;
		return;
	}

}

/**
 * This function provide an easy print to any uart line.
 */
int uart_printf( UART_HandleTypeDef *uart_handle, uart_buffer_t *tx_buffer, const char *__restrict format, ... )
{
	int32_t len;
	uint8_t err;
	va_list argptr;
	va_start( argptr, format );
	len = vsnprintf( (char *) tx_buffer->base, tx_buffer->size, format, argptr );
	va_end( argptr );

	// printf-like functions return negative values on error
	if( len < 0 )
	{
		// error occurred
		return len;
	}
	err = HAL_UART_Transmit( uart_handle, tx_buffer->base, len, len * 10 );
	len = err ? -1 : len;
	return len;
}
