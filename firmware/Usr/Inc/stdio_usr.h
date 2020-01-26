/*
 * lib_stdio.h
 *
 *  Created on: Dec 23, 2019
 *      Author: rg
 */

#ifndef INC_STDIO_USR_H_
#define INC_STDIO_USR_H_

#include "main_usr.h"
#include "lib_uart.h"
#include "global_config.h"

int uart_printf(UART_HandleTypeDef *uart_handle, uart_buffer_t *tx_buffer, const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 3, 4)) );
int debug(const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 1, 2)) );
int reply(const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 1, 2)) );
int errreply(const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 1, 2)) );

void ok(void);
void fail(void);
void print_config(runtime_config_t *rc);


#endif /* INC_STDIO_USR_H_ */
