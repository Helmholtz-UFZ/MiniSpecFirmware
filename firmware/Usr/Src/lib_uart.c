/*
 * usart_usr.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include <lib_uart.h>
#include "string.h"
#include <stdarg.h>
#include <stdio.h>

rxtx_config_t rxtx = {
		.wakeup = false,
		.cmd_bytes = 0,
		.use_debugprints = false
};

/* Memory blocks and buffer for transmitting and receiving
 * via uart interface.*/
static uint8_t rx_mem_block3[UART_RX_BUFFER_SZ];
static uint8_t tx_mem_block3[UART_TX_BUFFER_SZ];
uart_buffer_t rxtx_rxbuffer = { .size = UART_RX_BUFFER_SZ, .base = rx_mem_block3 };
uart_buffer_t rxtx_txbuffer = { .size = UART_TX_BUFFER_SZ, .base = tx_mem_block3 };

/**
 * Overwrite weak _write function, to make printf print to serial.
 **/
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
 * Init the uart interfaces to our needs.
 * Mainly set the trigger char to carriage return ('\r')
 * and enable the character match interrupt.
 * If then a carriage return is received the character match (CR)
 * interrupt is generated.
 *
 * Other char's does not trigger any interrupt as DMA is
 * used to transfer these from the uart receive register to memory.
 */
void rxtx_init(void) {
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

	HAL_NVIC_ClearPendingIRQ(RXTX_IRQn);
	HAL_NVIC_EnableIRQ(RXTX_IRQn);
}

/**
 * Resets the uart buffer and restart listening in DMA mode.
 */
void rxtx_restart_listening(void) {
	HAL_UART_AbortReceive(&hrxtx);
	memset(rxtx_rxbuffer.base, 0, rxtx_rxbuffer.size);
	HAL_UART_Receive_DMA(&hrxtx, rxtx_rxbuffer.base, rxtx_rxbuffer.size);
}

/**
 * This function provide an easy printf style writing
 * to any available uart line.
 *
 *\param uart_handle 	A pointer to a U(S)ART handle
 *\param tx_buffer	A pointer to a buffer to work with.
 *\param format 	A printf-style format string.
 *
 */
int uart_printf(UART_HandleTypeDef *uart_handle, uart_buffer_t *tx_buffer, const char *__restrict format, ...) {
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

/**
 * Print a string to uart if tx_dbgflg is not zero, otherwise
 * the function do nothing and return 0.
 *
 * Act like printf() but put the string 'dbg: ' upfront the message,
 * so any format strings printf() can eat are possible.
 */
int debug(const char *__restrict format, ...) {
	int len;
	if (rxtx.use_debugprints) {
		va_list args;
		printf("dbg: ");
		va_start(args, format);
		len = vprintf(format, args);
		va_end(args);
		return len;
	} else {
		return 0;
	}
}

/**
 * Print a string to rxtx uart
 * Act like printf() but put the string '->' upfront the message,
 * so any format strings printf() can eat are possible.
 */
int reply(const char *__restrict format, ...) {
	int len;
	va_list args;
	printf("-> ");
	va_start(args, format);
	len = vprintf(format, args);
	va_end(args);
	return len;
}
/**
 * Print a string to rxtx uart
 * Act like printf() but put the string '->' upfront the message,
 * so any format strings printf() can eat are possible.
 */
int errreply(const char *__restrict format, ...) {
	int len;
	va_list args;
	printf("ERR: ");
	va_start(args, format);
	len = vprintf(format, args);
	va_end(args);
	return len;
}
