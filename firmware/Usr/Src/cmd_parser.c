/*
 * cmd_parser.c
 *
 *  Created on: Apr 18, 2019
 *      Author: rg
 */

#include "cmd_parser.h"
#include "main.h"
#include "string.h"

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

	str = "stream\r";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_STREAM_START;
		return;
	}

	str = "end\r";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_STREAM_END;
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

#if DBG_CODE
	str = "#test\r";
	alias = "#t\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_DBGTEST;
		return;
	}
#endif

	str = "#debug\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0) {
		extcmd.cmd = USR_CMD_DEBUG;
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

	str = "itime?\r";
	alias = "i?\r";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_READ_ITIME;
		return;
	}

	str = "itime=";
	alias = "i=";
	sz = strlen(str);
	aliassz = strlen(alias);
	if (memcmp(buffer, str, sz) == 0 || memcmp(buffer, alias, aliassz) == 0) {
		extcmd.cmd = USR_CMD_WRITE_ITIME;
		/* Set pointer to char after the '=' */
		str = (char*) memchr(buffer, '=', sz) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "format=";
	sz = strlen(str);
	if (memcmp(buffer, str, sz) == 0) {
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
