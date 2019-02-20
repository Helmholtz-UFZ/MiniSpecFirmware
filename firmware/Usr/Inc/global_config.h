/*
 * global_config.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#ifndef GLOBAL_CONFIG_H_
#define GLOBAL_CONFIG_H_

/**This is (also) included by main.h
 * Avoid including high level stuff here.
 * Otherwise conflicts arise, as the HAL
 * isn't initialized yet. **/
#include "main_usr.h"
#include "stdbool.h"
#include "stdint.h"
#include "helper_defines.h"



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

/** max 1 sec, even this is optimistic to get correct data. */
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
#define DBG_SEND_ALL		ON

/* If defined no HW detection (Pin high) is done
 * by SD_initialize() in sd_diskio.c */
#define DISABLE_SD_INIT

#endif /* GLOBAL_CONFIG_H_ */
