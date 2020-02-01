/*
 * lib_stdio.h
 *
 *  Created on: Dec 23, 2019
 *      Author: Bert Palm
 */

#ifndef INC_LOGGING_H_
#define INC_LOGGING_H_

#include <globalconfig.h>
#include <mainloop.h>
#include <rxtx.h>

int uart_printf(UART_HandleTypeDef *uart_handle, uart_buffer_t *tx_buffer, const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 3, 4)) );
int debug(int8_t lvl, const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 2, 3)) );
int reply(const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 1, 2)) );
int errreply(const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 1, 2)) );

#endif /* INC_LOGGING_H_ */
