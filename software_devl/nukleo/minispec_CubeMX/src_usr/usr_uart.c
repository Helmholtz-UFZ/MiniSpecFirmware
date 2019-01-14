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
#include <stdio.h>

volatile bool uart1_CR_recvd;
volatile uint16_t uart1_cmd_bytes;

static uint8_t rx_mem_block3[UART_DEFAULT_RX_BUFFER_SZ];
static uint8_t tx_mem_block3[UART_DEFAULT_TX_BUFFER_SZ];
uart_buffer_t uart1_rx_buffer =
{ UART_DEFAULT_RX_BUFFER_SZ, rx_mem_block3 };
uart_buffer_t uart1_tx_buffer =
{ UART_DEFAULT_TX_BUFFER_SZ, tx_mem_block3 };

/**
 * Init the all used uart interfaces to our needs.
 * May overwrite some preferences CubeMx made.
 */
void usart1_init( void )
{
	/* Enable character match (CM) IR for
	 * carriage return (CR) char.*/

	uint32_t cr1 = 0;
	char trigger_char;

	//save cr1 status
	cr1 = USART1->CR1;

	// reset RE and UE bit
	USART1->CR1 &= ~(USART_CR1_RE | USART_CR1_UE);

	//modifying ADD[7:0]
	trigger_char = '\r';
	USART1->CR2 |= (USART_CR2_ADD_Msk & (trigger_char << USART_CR2_ADD_Pos));

	// restore RE and UE bit
	USART1->CR1 = cr1;

	//enable char match IR-Flag
	USART1->CR1 |= USART_CR1_CMIE;

	uart1_CR_recvd = 0;
	uart1_cmd_bytes = 0;
}

/**
 * resets the uart buffer and restart listening
 * with DMA.
 */
void usart1_receive_handler( void )
{
	if( uart1_CR_recvd )
	{
		uart1_CR_recvd = 0;

		// restart listening
		HAL_UART_AbortReceive( &huart1 );
		memset( uart1_rx_buffer.base, 0, uart1_cmd_bytes );
		uart1_cmd_bytes = 0;
		HAL_UART_Receive_DMA( &huart1, uart1_rx_buffer.base, uart1_rx_buffer.size );
	}
}

/**
 * This function provide an easy print to any available uart line.
 *
 *\param uart_handle 	A pointer to a U(S)ART handle
 *\param tx_buffer	A pointer to a buffer where the following arguments are copied to and send from.
 *\param format 	A printf-style format string.
 *
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
