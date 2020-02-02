/*
 * mainloop.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include "main.h"

#include "mainloop.h"

#include "globalconfig.h"
#include "defines.h"
#include "wakeup.h"
#include "sysrc.h"
#include "logging.h"
#include "sd.h"
#include "sensor.h"
#include "rxtx.h"
#include "cpu.h"
#include "power.h"

#include <stdbool.h>


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
	sd_write_reset_info();
	read_config_from_SD(&rc);

	mode_switch(&rc);
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
	if (!wakeup.alarmA && !wakeup.triggerPin && !wakeup.cmd) {
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

	debug(3, "(main): alarm/trigger/cmd: %i %i %i\n", wakeup.alarmA, wakeup.triggerPin, wakeup.cmd);

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


// if this is run by anyone we check if the
// necessary changes was made in the STM32_HAL
//
// this is at the end to make the upper includes
// be more explicit and does not get `disturbed`
// by the includes that are needed for the checks
#include "cube_checks.h"
