/*
 * lib_stdio.c
 *
 *  Created on: Dec 23, 2019
 *      Author: Bert Palm
 */


#include <logging.h>
#include <mainloop.h>
#include <rxtx.h>

#include <stdarg.h>
#include <stdio.h>

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
int debug(int8_t lvl, const char *__restrict format, ...) {
	int len;
	if (lvl <= rc.debuglevel) {
		va_list args;
		printf("DBG-%d:", lvl);
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

