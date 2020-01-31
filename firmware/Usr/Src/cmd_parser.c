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


static bool _parsecmd(uint8_t *buf, char *cmd, char *alias){
	uint8_t sz = strlen(cmd);
	if (memcmp(buf, cmd, sz) == 0) {
		return true;
	}
	if (alias){
		sz = strlen(alias);
		if (memcmp(buf, alias, sz) == 0) {
			return true;
		}
	}
	return false;
}

static void _set_argbuf(uint8_t *buf, char *cmd){
	/* Search the '=' */
	char *p = (char*) memchr(buf, '=', strlen(cmd));

	/* Copy the argstr to the arg_buffer, so we can reset
	 * the receive buffer and listen on the rx-line again. */
	if (p){
		strncpy(extcmd.arg_buffer, p+1, ARGBUFFSZ);
	}
}

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
 */
void parse_extcmd(uint8_t *buffer, uint16_t size) {
	UNUSED(size);

	/* init cmd structure */
	extcmd.cmd = USR_CMD_UNKNOWN;
	memset(&extcmd.arg_buffer, 0, ARGBUFFSZ);

	// ============================================
	// cmds without arguments (terminated by `\r`)
	// ============================================

	if(_parsecmd(buffer, "measure\r", "m\r")){
		extcmd.cmd = USR_CMD_SINGLE_MEASURE_START;
		return;
	}

	if(_parsecmd(buffer, "multimeasure\r", "mm\r")){
		extcmd.cmd = USR_CMD_MULTI_MEASURE_START;
		return;
	}

	if(_parsecmd(buffer, "getdata\r", "gd\r")){
		extcmd.cmd = USR_CMD_GET_DATA;
		return;
	}

	if(_parsecmd(buffer, "storeconf\r", "stcf\r")){
		extcmd.cmd = USR_CMD_STORE_SDCONFIG;
		return;
	}

	if(_parsecmd(buffer, "readconf\r", "rdcf\r")){
		extcmd.cmd = USR_CMD_READ_SDCONFIG;
		return;
	}

	if(_parsecmd(buffer, "#test\r", NULL)){
		extcmd.cmd = USR_CMD_DBGTEST;
		return;
	}

	if(_parsecmd(buffer, "version\r", NULL)){
		extcmd.cmd = USR_CMD_VERSION;
		return;
	}

	if(_parsecmd(buffer, "help\r", "h\r")){
		extcmd.cmd = USR_CMD_HELP;
		return;
	}

	if(_parsecmd(buffer, "rtc?\r", NULL)){
		extcmd.cmd = USR_CMD_GET_RTC_TIME;
		return;
	}

	if(_parsecmd(buffer, "config?\r", "c?\r")){
		extcmd.cmd = USR_CMD_GET_CONFIG;
		return;
	}

	if(_parsecmd(buffer, "configsd?\r", "c?sd\r")){
		extcmd.cmd = USR_CMD_PRINT_SDCONFIG;
		return;
	}

	if(_parsecmd(buffer, "itime?\r", "i?\r")){
		extcmd.cmd = USR_CMD_GET_ITIME;
		return;
	}

	// ============================================
	// cmds with arguments are split/termiated by `=`
	// ============================================

	if(_parsecmd(buffer, "debug=", "dbg=")){
		_set_argbuf(buffer, "debug=");
		extcmd.cmd = USR_CMD_DEBUG;
		return;
	}

	if(_parsecmd(buffer, "itime=", "i=")){
		_set_argbuf(buffer, "itime=");
		extcmd.cmd = USR_CMD_SET_ITIME;
		return;
	}

	if(_parsecmd(buffer, "itimeindex=", "ii=")){
		_set_argbuf(buffer, "itimeindex=");
		extcmd.cmd = USR_CMD_SET_ITIME_INDEX;
		return;
	}

	if(_parsecmd(buffer, "iterations=", "N=")){
		_set_argbuf(buffer, "iterations=");
		extcmd.cmd = USR_CMD_SET_MULTI_MEASURE_ITERATIONS;
		return;
	}

	if(_parsecmd(buffer, "format=", "f=")){
		_set_argbuf(buffer, "format=");
		extcmd.cmd = USR_CMD_SET_FORMAT;
		return;
	}

	if(_parsecmd(buffer, "rtc=", NULL)){
		_set_argbuf(buffer, "rtc=");
		extcmd.cmd = USR_CMD_SET_RTC_TIME;
		return;
	}

	if(_parsecmd(buffer, "mode=", NULL)){
		_set_argbuf(buffer, "mode=");
		extcmd.cmd = USR_CMD_SET_MODE;
		return;
	}
}

/** Parse any +- integer from arg_buffer to arg.
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

	if (rc->mode == MODE_OFF || rc->mode == MODE_TRIGGERED) {
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

	if (rc->mode == MODE_ENDLESS) {
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
