/*
 * globalconfig.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#ifndef GLOBAL_CONFIG_H_
#define GLOBAL_CONFIG_H_

/** The minimal possible integration time is limited by the sensor and the
 *  clock frequency.
 *  The minimal high period of ST-pulse:   6 * CLK_freq
 *  The minimal low  period of ST-pulse: 375 * CLK_freq
 *  The minimal full period of ST-pulse: 381 * CLK_freq
 *
 *  We have a CLK_freq of 1Mhz
 *  The integration time is 'high period of ST-pulse' + 48 CLK_cycles
 *
 *  So the minimal integration time we can choose is:
 *  6 CLK_cycles + 48 CLK_cycles = 54 CLK_cycles
 *  6 us         + 48 us         = 54 us
 */
#define MIN_INTERGATION_TIME		(54)

/** max 2 sec, even this is optimistic to get correct data. */
#define MAX_INTERGATION_TIME		(1000000)
#define DEFAULT_INTEGRATION_TIME	(1100)

/** The number of Sensor-CLK-periods which
 *  are added to the ST-high-pulse.*/
#define ITIME_CORRECTION		(48)

/** The number of pixels the sensor provide.*/
#define MSPARAM_PIXEL			(288)

/** The number of TRG pulses until (including)
 *  first valid value is send */
#define MSPARAM_FIRST_VALID		(89)

/** The number of TRG-pulses before EOS occur */
#define TRG_TO_EOS			((MSPARAM_FIRST_VALID) + (MSPARAM_PIXEL))

/** start capturing this many values earlier.
 * before the first valid pixel. zero means
 * with the first valid. negative means later.
 * default: 5 */
#define START_FIRST_EARLY			(5)

/** end capturing relative to EOS.
 * 0 means first edge after EOS.
 * default: 5 */
#define END_LAST_LATE				(5)

/*
 * To test, watch the TEST signal on pin PA11 with the oszilloscope and
 * set START_FIRST_EARLY and/or END_LAST_LATE to zero.
 * than the rising edge should be at the first valid VID pixel
 * (synchron with rising edge of TRG) and the falling edge should be on
 * the rising edge of TRG right *after* the last valid pixel.
 *
 * bare in mind, that EOS disable data-capturing and TRG-counting.
 *
 * If possible use a 2nd channel and examine the sensors video signal.
 * It helps to set the integration time very high (~500 milli) and/or use
 * strong light to see the valid pixel range on this channel.
 */

/* start capturing with this TRG count */
#define CAPTURE_PXL_ST		((MSPARAM_FIRST_VALID) - (START_FIRST_EARLY))

/* end capturing with this TRG count. If EOS occur before capturing will end
 * with EOS. */
#define CAPTURE_PXL_END		((TRG_TO_EOS) + (END_LAST_LATE) )

/**
 * The time in us after the safty timer throw
 * an IR and stop a failed measurement.
 * measured from falling edge of ST signal.
 * 1MHz -> 1us: 1us * (89+288) <<! delay
 * (1Mhz) default: 500.
 */
#define SAFTY_TIMER_DELAY		(500)

/* prevent CubeMX do too special stuff */
#define DO_NOT_USE_HAL_IRQ_HANDLER

/* send all data/samples/pixel that we recorded, particularly the data
 * that is lying in the data buffer before the valid data. If off we
 * only send the valid samples (for DATA_FORMAT_ASCII only)*/
#define DBG_SEND_ALL		0

/* Enables Debug code*/
#define DBG_CODE			1

/* Set debug print ON/OFF on initial startup */
#define DEBUG_DEFAULT_LVL		1

/* Start Mode*/
#define START_MODE				0

/* If 0 all code that access the SD card is disabled. */
#define HAS_SD	 			1

/* if 0 Powerswitch is always on */
#define USE_POWER_SWITCH	1

/* If an error occur in the SD driver sometimes it helps
 * to re-initialize the HAL and FAT driver. This threshold
 * defines how often we try this, before we go home and cry. */
#define SD_MAX_REINIT_DRIVER	10

/* Base File Name and Extension of the measurement file.
 * The base name is extended by a underscore and a number
 * and its extension. The numbering starts with zero.
 * When the file size is grater than SD_MAX_FILESIZE the
 * number is incremented. */
#define SD_FILE_BASENAME		"MSPEC"
#define SD_FILE_EXTENSION		"CSV"


#define SD_CONFIGFILE_NAME		"CONFIG.TXT"

/*Maximum file size in Byte (default 10MB)*/
#define SD_MAX_FILESIZE			(10 * 1024 * 1024)

/* If defined: HW detection (Pin high/low) is done
 * by BSP_PlatformIsDetected() in fatfs_platform.c
 *
 * Attention: DISABLE_SD_INIT is not the correct define
 * for this purpose as this will also disable the HAL
 * initialization of the SD.
 *
 * If the Setup use the HW detection, than a plugged SD
 * connects the SD_DETECT_IN and _OUT. OUT is tied to GND
 * and _IN has a pullup. This means:
 * 	SD plugged   : SD_DETECT_PIN is RESET (low)
 * 	SD unplugged : SD_DETECT_PIN is SET (high) */
#define USE_SD_HW_DETECTION		0


/* Minimal possible repetition of measurements by setting
 * the interval which updates timerA.
 * Take maximal integration time and processing in account.
 * In seconds. default=11*/
#define MIN_IVAL		5

/*
 * Define the Uart Instance that is used to communicate
 * with the device. The instance is used for printf(),
 * debug() and all UART send/receive commands.
 * For a more specific (eg. use differnet instances for
 * debug() and normal usr commands) configuration, see
 * the usart_usr.h
 *
 * implemented choices are
 * 1: Uart1  (use  PB6/PB7  -> tx: CN10-Pin17, rx: CN7-Pin21)
 * 4: Uart4 (use PC10/PC11 -> tx: CN7-Pin1,   rx: CN7-Pin2)
 *
 * uart1 was default in HW-Version 2.0
 * uart4 is default in new HW-Version 2.1
 */
#define UART_INSTANCE_NR			1


#include "stdint.h"
#if MAX_INTERGATION_TIME > INT32_MAX
#error "MAX_INTERGATION_TIME: the absolut maximum value is reached"
#endif

#ifndef __FIRMWARE_VERSION
#error "firmware version undefined."
// Define as CTAG like so: -D__FIRMWARE_VERSION="$(shell git describe --always -dirty)"
// in AC6: project->properties->C/C++build->settings
//             ->tool settings->preprocessor->defined symbols
#endif

#endif /* GLOBAL_CONFIG_H_ */
