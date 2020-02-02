/*
 * cmd_handler.h
 *
 *  Created on: Feb 2, 2020
 *      Author: rg
 */

#ifndef INC_CMD_HANDLER_H_
#define INC_CMD_HANDLER_H_

/**
 * the data format specifies the datatype of the
 * arguments and the data to/from external system.
 **/
typedef enum {
	DATA_FORMAT_BIN = 0,
	DATA_FORMAT_ASCII = 1,

} dataformat_t;

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

void extcmd_handler(void);

#endif /* INC_CMD_HANDLER_H_ */
