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

	// getter
	USR_CMD_GET_DATA,
	USR_CMD_GET_ITIME,
	USR_CMD_GET_RTC_TIME,
	USR_CMD_GET_CONFIG,

	//setter
	USR_CMD_SET_FORMAT, // 0 - raw/bin or 1 - ascii
	USR_CMD_SET_SENSOR, // UNUSED (for future use) - choose if sens 1 or sens 2
	USR_CMD_SET_ITIME,
	USR_CMD_SET_ITIME_INDEX,
	USR_CMD_SET_AUTOADJUST_PARAMS,
	USR_CMD_SET_MULTI_MEASURE_ITERATIONS,
	USR_CMD_SET_RTC_TIME,
	USR_CMD_SET_MODE,

	// other
	USR_CMD_VERSION,
	USR_CMD_HELP,
	USR_CMD_STORE_SDCONFIG,
	USR_CMD_READ_SDCONFIG,
	USR_CMD_PRINT_SDCONFIG,
	USR_CMD_TEST_AUTOADJUST,

	//debug
	USR_CMD_DEBUG,
	USR_CMD_DBGTEST,

} usr_cmd_enum_t;

// todo future: store COMAND_NR, comandString, alias, help-txt in one
// dictionary-like structure
#define HELPSTR  "HELP\n" \
	"h       - print this help\n"\
	"version - print the firmware version\n"\
	"stcf    - store config to sd\n"\
	"rdcf    - read back the config from the sd into the system\n"\
/*	"#test   - run a test command"         hidden feature*/\
	"m       - measure\n"\
	"mm      - multimeasure\n"\
	"gd      - print (last) data or error\n"\
	"aa      - test auto-adjust\n"\
	"i?      - print intergration time\n"\
	"rtc?    - print current time\n"\
	"c?      - print system config\n"\
	"c?sd    - print the config, that is stored on sd\n"\
	"dbg=    - set debug message verbosity. 0: off, 1: some, 2:many, 3+:all\n"\
	"aa=     - set auto-adjust params\n"\
	"i=      - set intergration time (negativ values set to auto-adjust)\n"\
	"ii=     - set index for setting inegration time\n"\
	"N=      - set iterations per measurement (for mm)\n"\
	"rtc=    - set current time format: '20YY-MM-DDTHH:MM:SS'\n"\
	"format={0/1} \n"\
	"        - set format bin/ascii \n"\
	"mode={0 (off) / 1,IVAL / 2,IVAL,START,STOP / 3 (triggered)} \n"\
    "        - set the mode with IVAL,STRT,STOP format: 'HH:MM:SS' \n"\
	"\n"

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
int8_t argparse_nr(int32_t *nr);
int8_t argparse_nrs(int32_t *nr1, int32_t *nr2);
int8_t argparse_str(char **str);
int8_t parse_ival(char *str, runtime_config_t* rc);



#endif /* INC_CMD_PARSER_H_ */
