/*
 * main_usr.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */


#include "mainloop.h"
#include "wakeup.h"
#include "sysrc.h"
#include "rxtx.h"


static void _sleep(void);
static void _run(void);


runtime_config_t rc = {
		.iterations = 1,
		.itime = {DEFAULT_INTEGRATION_TIME, 0,},
		.itime_index = 0,
		.aa_lower = 33000,
		.aa_upper = 54000,
		.mode = START_MODE,
		.start = {.Hours = 99, .Minutes = 99, .Seconds = 99},
		.end = {.Hours = 99, .Minutes = 99, .Seconds = 99},
		.ival = {.Hours = 99, .Minutes = 99, .Seconds = 99},
		.next_alarm = {.Hours = 99, .Minutes = 99, .Seconds = 99},
		.debuglevel = DEBUG_DEFAULT_LVL,
		.format = DATA_FORMAT_ASCII,
		.sleeping = false,
		.trigger = false,
};


void usr_hw_init(void) {
	/* sa. Errata 2.1.3. or Arm ID number 838869 */
	uint32_t *ACTLR = (uint32_t *) 0xE000E008;
	*ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk;

	sensor_deinit();
	rxtx_init();
	rxtx_restart_listening();
}


void run_init(void) {
	power_switch_EN(ON);
	usr_hw_init();

	// inform user
	reply("\nstart\n");
	reply("firmware: %s\n", __FIRMWARE_VERSION);

	// read/write info from/to sd
	inform_SD_reset();
	read_config_from_SD(&rc);

	init_mode(&rc);
}


void run(void) {

	while(true)
	{
		_sleep();
		_run();
	}
}


static void _sleep(void){
	/* Uart IR is enabled only during (light) sleep phases.
	 * If a uart IR occur during the IR is disabled, it will
	 * come up, immediately after enabling it agian.
	 * (see next line) */
	__HAL_UART_ENABLE_IT(&hrxtx, UART_IT_CM);
	if (!rxtx.wakeup && !rtc.alarmA_wakeup) {
		cpu_enter_LPM();
	}
	__HAL_UART_DISABLE_IT(&hrxtx, UART_IT_CM);
}


void _run(void) {

	if (hrxtx.ErrorCode) {
		/* See stm32l4xx_hal_uart.h for ErrorCodes.
		 * Handle Uart Errors immediately because every further
		 * send (e.g. printf) would overwrite the Error Code.*/
		rxtx_restart_listening();
	}

	debug(3, "(main): rxtx,alarm,trigger: %i %i %i\n", rxtx.wakeup, rtc.alarmA_wakeup, rc.trigger);

	if (wakeup.triggerPin) {
		wakeup.triggerPin = false;
		if (rc.mode == MODE_TRIGGERED) {
			wakeup_pintrigger_handler();
		}
	}

	if (wakeup.alarmA) {
		wakeup.alarmA = false;
		if (rc.mode == MODE_ENDLESS || rc.mode == MODE_STARTEND) {
			wakeup_alarm_handler();
		}
	}

	if (wakeup.cmd) {
		wakeup.cmd = false;
		wakeup_cmd_handler();
	}
}










