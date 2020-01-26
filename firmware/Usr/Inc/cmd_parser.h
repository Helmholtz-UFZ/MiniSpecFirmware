/*
 * cmd_parser.h
 *
 *  Created on: Apr 18, 2019
 *      Author: rg
 */

#ifndef INC_CMD_PARSER_H_
#define INC_CMD_PARSER_H_

#include <lib_uart.h>
#include "global_config.h"
#include "main_usr.h"

/**
 * enum for the defined commands
 */
typedef enum
{
	USR_CMD_UNKNOWN,

	// run
	USR_CMD_SINGLE_MEASURE_START,
	USR_CMD_MULTI_MEASURE_START,
	USR_CMD_STREAM_START,
	USR_CMD_STREAM_END,

	// getter
	USR_CMD_GET_DATA,
	USR_CMD_GET_ITIME,
	USR_CMD_GET_INDEXED_ITIME,
	USR_CMD_GET_RTC_TIME,
	USR_CMD_GET_INTERVAL,
	USR_CMD_GET_CONFIG,

	//setter
	USR_CMD_SET_FORMAT, // 0 - raw/bin or 1 - ascii
	USR_CMD_SET_SENSOR, // UNUSED (for future use) - choose if sens 1 or sens 2
	USR_CMD_SET_ITIME,
	USR_CMD_SET_ITIME_AUTO,
	USR_CMD_SET_ITIME_INDEX,
	USR_CMD_SET_MULTI_MEASURE_ITERATIONS,
	USR_CMD_SET_RTC_TIME,
	USR_CMD_SET_INTERVAL,

	// other
	USR_CMD_VERSION,
	USR_CMD_HELP,
	USR_CMD_STORE_CONFIG,
	USR_CMD_READ_CONFIG,

	//debug
	USR_CMD_DEBUG,
	USR_CMD_DBGTEST,

} usr_cmd_enum_t;

#define HELPSTR  "" \
	"h      - print this help\n"\
	"version- print the firmware version"\
	"stcf   - store config to sd\n"\
	"rdcf   - read config from sd\n"\
/*	"#debug - toggle dbg messages"        hidden feature*/\
/*	"#test  - run a test command"         hidden feature*/\
	"m      - measure\n"\
	"mm     - multimeasure\n"\
	"stream - streaming data\n"\
	"end    - end stream\n"\
	"gd     - get (last) data or error\n"\
	"i?     - get intergration time\n"\
	"ii?    - get (current) integration time index\n"\
	"rtc?   - get current time\n"\
	"ival?  - get interval\n"\
	"c?     - get config\n"\
	"i=     - set intergration time\n"\
	"ii=    - set index to set inegration time for mm\n"\
	"N=     - set iterations per measurement for mm\n"\
	"rtc=   - set current time 20YY-MM-DDTHH:MM:SS\n"\
	"format={0/1} - set format bin/ascii\n"\
	"ival={0 / 1,IVAL / 2,IVAL,START,STOP} \n"\
    "  - set interval with IVAL,STRT,STOP = HH:MM:SS\n\n"

/** Size of the argument buffer in usr_cmd_typedef
 * must not exceed UART_RX_BUFFER_SZ */
#define ARGBUFFSZ	100
#if ARGBUFFSZ > UART_RX_BUFFER_SZ
#error "Buffer to big"
#endif

typedef struct
{
	/* Holds the actual command - nevertheless of parsing errors.*/
	usr_cmd_enum_t cmd;

	/* Holds a pointer to the argument string after the '=' sign in
	 * a user command. */
	char arg_buffer[ARGBUFFSZ];

} usr_cmd_typedef;

extern usr_cmd_typedef extcmd;

void parse_extcmd(uint8_t *buffer, uint16_t size);
int8_t argparse_nr(uint32_t *nr);
int8_t argparse_str(char **str);
int8_t parse_ival(char *str, runtime_config_t* rc);



#endif /* INC_CMD_PARSER_H_ */
