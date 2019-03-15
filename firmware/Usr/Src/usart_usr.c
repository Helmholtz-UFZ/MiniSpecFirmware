/*
 * usart_usr.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include "usart_usr.h"
#include "string.h"
#include <stdarg.h>
#include <stdio.h>

volatile bool rxtx_CR_recvd;
volatile uint16_t rxtx_cmd_bytes;

uint8_t tx_dbgflg = 1;

static uint8_t rx_mem_block3[UART_DEFAULT_RX_BUFFER_SZ];
static uint8_t tx_mem_block3[UART_DEFAULT_TX_BUFFER_SZ];
uart_buffer_t rxtx_rxbuffer = { UART_DEFAULT_RX_BUFFER_SZ, rx_mem_block3 };
uart_buffer_t rxtx_txbuffer = { UART_DEFAULT_TX_BUFFER_SZ, tx_mem_block3 };

/** Overwrite weak _write function, to make printf print to serial. */
int _write(int file, char *ptr, int len) {
	switch (file) {
	case STDOUT_FILENO: /*stdout*/
		HAL_UART_Transmit(&hprintf, (uint8_t *) ptr, len, 0xFFFF);
		break;
	case STDERR_FILENO: /* stderr */
		HAL_UART_Transmit(&hprintf, (uint8_t *) "error: ", 7, 0xFFFF);
		HAL_UART_Transmit(&hprintf, (uint8_t *) ptr, len, 0xFFFF);
		break;
	default:
		return -1;
	}
	return len;
}

/**
 * Init the all used uart interfaces to our needs.
 * May overwrite some preferences CubeMx made.
 */
void rxtx_init(void) {
	/* Enable character match (CM) IR for
	 * carriage return (CR) char.*/

	uint32_t cr1 = 0;
	char trigger_char;

	//save cr1 status
	cr1 = RXTX->CR1;

	// reset RE and UE bit
	RXTX->CR1 &= ~(USART_CR1_RE | USART_CR1_UE);

	//modifying ADD[7:0]
	trigger_char = '\r';
	RXTX->CR2 |= (USART_CR2_ADD_Msk & (trigger_char << USART_CR2_ADD_Pos));

	// restore RE and UE bit
	RXTX->CR1 = cr1;

	//enable char match IR-Flag
	RXTX->CR1 |= USART_CR1_CMIE;

	rxtx_CR_recvd = 0;
	rxtx_cmd_bytes = 0;
}

/**
 * resets the uart buffer and restart listening
 * with DMA.
 */
void rx_handler(void) {
	if (rxtx_CR_recvd) {
		rxtx_CR_recvd = 0;

		// restart listening
		HAL_UART_AbortReceive(&hrxtx);
		memset(rxtx_rxbuffer.base, 0, rxtx_cmd_bytes);
		rxtx_cmd_bytes = 0;
		HAL_UART_Receive_DMA(&hrxtx, rxtx_rxbuffer.base, rxtx_rxbuffer.size);
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
int uart_printf(UART_HandleTypeDef *uart_handle, uart_buffer_t *tx_buffer,
		const char *__restrict format, ...) {
	int32_t len;
	uint8_t err;
	va_list argptr;
	va_start(argptr, format);
	len = vsnprintf((char *) tx_buffer->base, tx_buffer->size, format, argptr);
	va_end(argptr);

	// printf-like functions return negative values on error
	if (len < 0) {
		// error occurred
		return len;
	}
	err = HAL_UART_Transmit(uart_handle, tx_buffer->base, len, len * 10);
	len = err ? -1 : len;
	return len;
}

int debug(const char *__restrict format, ...) {
	int len;
	if (tx_dbgflg) {
		va_list args;
		va_start(args, format);
		printf("dbg: ");
		len = printf(format, args);
		va_end(args);
		return len;
	} else {
		return 0;
	}
}

int tx_printf(const char *__restrict format, ...) {
	int32_t len;
	uint8_t err;
	va_list argptr;
	va_start(argptr, format);
	len = vsnprintf((char *) rxtx_txbuffer.base, rxtx_txbuffer.size, format,
			argptr);
	va_end(argptr);

	// printf-like functions return negative values on error
	if (len < 0) {
		// error occurred
		return len;
	}
	err = HAL_UART_Transmit(&hrxtx, rxtx_txbuffer.base, len, len * 10);
	len = err ? -1 : len;
	return len;
}

