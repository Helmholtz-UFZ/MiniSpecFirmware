/*
 * cmd_parser.c
 *
 *  Created on: Apr 18, 2019
 *      Author: rg
 */

#include "cmd_parser.h"
#include "main.h"
#include "string.h"
#include "stdio.h"

usr_cmd_typedef extcmd;
/*
 * check and parse for an command and set the
 * variable usrcmd if any command was found.
 *
 * The command have to be in the beginning
 * of the given buffer.
 *
 * @param buffer the buffer to check
 * @param size unused
 *
 * todo (future release) check the whole buffer up to size
 */
void parse_extcmd(uint8_t *buffer, uint16_t size) {
	UNUSED(size);
	char *str, *alias;
	uint16_t sz, aliassz;

	/* init cmd structure */
	extcmd.cmd = USR_CMD_UNKNOWN;
	memset(&extcmd.arg_buffer, 0, ARGBUFFSZ);

	str = "measure\r";
	alias = "m\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_SINGLE_MEASURE_START;
		return;
	}

	str = "multimeasure\r";
	alias = "mm\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_MULTI_MEASURE_START;
		return;
	}

	str = "getdata\r";
	alias = "gd\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_GET_DATA;
		return;
	}

	str = "storeconf";
	alias = "stcf";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_STORE_CONFIG;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "readconf";
	alias = "rdcf";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_READ_CONFIG;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "#debug\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_DEBUG;
		return;
	}

	str = "#test\r";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_DBGTEST;
		return;
	}

	str = "version\r";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_VERSION;
		return;
	}

	str = "help\r";
	alias = "h\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_HELP;
		return;
	}

	str = "rtc?\r";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_GET_RTC_TIME;
		return;
	}

	str = "ival?\r";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_GET_INTERVAL;
		return;
	}

	str = "config?\r";
	alias = "c?\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_GET_CONFIG;
		return;
	}

	str = "itime?\r";
	alias = "i?\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_GET_ITIME;
		return;
	}

	str = "itimeindex?\r";
	alias = "ii?\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_GET_INDEXED_ITIME;
		return;
	}

	str = "itime=";
	alias = "i=";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_SET_ITIME;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "autoadjust\r";
	alias = "aa\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_SET_ITIME_AUTO;
		return;
	}


	str = "itimeindex=";
	alias = "ii=";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_SET_ITIME_INDEX;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "iterations=";
	alias = "N=";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_SET_MULTI_MEASURE_ITERATIONS;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "format=";
	alias = "f=";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_SET_FORMAT;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "rtc=";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_SET_RTC_TIME;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "ival=";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_SET_INTERVAL;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}
}

/** Parse natural number from arg_buffer to arg.
 *  Return 0 on success, otherwise non-zero */
int8_t argparse_nr(int32_t *nr) {
	int res;
	if (extcmd.arg_buffer[0] == 0) {
		/* buffer empty */
		return -1;
	}
	res = sscanf(extcmd.arg_buffer, "%ld", nr);
	if (res <= 0) {
		nr = 0;
		return res;
	}
	return 0;
}

/** Parse string from arg_buffer to arg.
 *  Return 0 on success, otherwise non-zero */
int8_t argparse_str(char **str) {
	UNUSED(str);
	if (extcmd.arg_buffer[0] == 0) {
		/* buffer empty */
		return -1;
	}
	*str = extcmd.arg_buffer;
	return 0;
}

/**
 * Parse 'mode,ival,start,end' from string
 * from arg_buffer and set the aprropiate
 * params in rc-struct.
 * If parsing fails, rc-struct is untouched.
 *
 * Return 0 on success,
 * Return 1 on parsing error
 * Return 2 on constrains error
 *
 * Note: Start time needs to be smaller than end time
 */
int8_t parse_ival(char *str, runtime_config_t* rc) {

	uint c = 99;
	RTC_TimeTypeDef iv, st, en, off;
	init_timetype(&off);

	/*parse mode */
	sscanf(str, "%u", &c);
	if (c > 3) {
		return 1;
	}

	rc->mode = c;

	if (rc->mode == IVAL_OFF || rc->mode == TRIGGERED) {
		rc->start = off;
		rc->end = off;
		rc->ival = off;
		return 0;
	}

	str++; //ignore c

	/*parse interval*/
	str = (char*) memchr(str, ',', 5);
	if (!str) {
		return 1;
	}
	str++; // ignore ','
	if (rtc_parse_time(str, &iv)) {
		return 1;
	}
	if (iv.Hours == 0 && iv.Minutes == 0 && iv.Seconds < MIN_IVAL) {
		return 1;
	}

	rc->ival = iv;

	if (rc->mode == IVAL_ENDLESS) {
		return 0;
	}

	/*parse start-time*/
	str = (char*) memchr(str, ',', 20);
	if (!str) {
		return 1;
	}
	str++; // ignore ','
	if (rtc_parse_time(str, &st)) {
		return 1;
	}
	/*parse end-time*/
	str = (char*) memchr(str, ',', 20);
	if (!str) {
		return 1;
	}
	str++; // ignore ','
	if (rtc_parse_time(str, &en)) {
		return 1;
	}
	/*check constrains*/
	if (!rtc_time_lt(&st, &en)) {
		return 2;
	}

	rc->start = st;
	rc->end = en;

	return 0;
}
