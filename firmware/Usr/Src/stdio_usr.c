/*
 * lib_stdio.c
 *
 *  Created on: Dec 23, 2019
 *      Author: rg
 */


#include "main_usr.h"
#include "lib_uart.h"
#include "stdio_usr.h"
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


/** print 'ok' */
void ok(void) {
	if (rc.format == DATA_FORMAT_ASCII) {
		reply("ok\n");
	}
}

/** print 'fail' */
void fail(void) {
	if (rc.format == DATA_FORMAT_ASCII) {
		errreply("argument error\n");
	}
}

void print_config(runtime_config_t *rc, char *name){
	// todo: mv this away from stdio_usr.c
	rtc_timestamp_t ts;

	reply("%s:\n", name);
	printf("mode: %u\n", rc->mode);
	printf("dbglvl: %u\n", rc->debuglevel);
	for (int i = 0; i < RCCONF_MAX_ITIMES; ++i) {
		if (i == rc->itime_index) {
			printf("itime[%u] = %ld  <---- ii (index)\n", i, rc->itime[i]);
			continue;
		}
		if (rc->itime[i] != 0) {
			printf("itime[%u] = %ld\n", i, rc->itime[i]);
		}
	}
	printf("ii: %u  (index)\n", rc->itime_index);
	printf("iter. per meas. [N]: %u\n", rc->iterations);
	printf("start time:      %02i:%02i:%02i\n", rc->start.Hours, rc->start.Minutes, rc->start.Seconds);
	printf("end time:        %02i:%02i:%02i\n", rc->end.Hours, rc->end.Minutes, rc->end.Seconds);
	printf("interval:        %02i:%02i:%02i\n", rc->ival.Hours, rc->ival.Minutes, rc->ival.Seconds);
	printf("next auto-meas.: %02i:%02i:%02i\n", rc->next_alarm.Hours, rc->next_alarm.Minutes, rc->next_alarm.Seconds);
	ts = rtc_get_now();
	printf("now:  20%02i-%02i-%02iT%02i:%02i:%02i\n", ts.date.Year, ts.date.Month, ts.date.Date, ts.time.Hours,
			ts.time.Minutes, ts.time.Seconds);
}
