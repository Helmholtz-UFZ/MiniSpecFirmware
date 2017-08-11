/*
 * global_config.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#ifndef GLOBAL_CONFIG_H_
#define GLOBAL_CONFIG_H_

// this should be as least as big that we can send one whole
// measurement data plus some meta data.
#define UART_DEFAULT_TX_BUFFER_SZ	(1024)

// small as we just need it for receiving user commands
#define UART_DEFAULT_RX_BUFFER_SZ	(128)

/** The delay between function call and ST goes high.
 *  This only is used by the timer, to ensure that the initial
 *  value is not zero*/
#define PRE_ST_DELAY		1

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

/* The number of pixels the sensor provide.*/
#define MSPARAM_PIXEL			(288)
#define MSPARAM_FISRT_VALID		(89)
#define MSPARAM_SAFETY_FRAME		(10)
#define MSPARAM_DEFAULT_INTTIME		(110000)

#define MSPARAM_CAPTURE_PXL_ST		(MSPARAM_FISRT_VALID - MSPARAM_SAFETY_FRAME)
#define MSPARAM_CAPTURE_PXL_END		(MSPARAM_FISRT_VALID + MSPARAM_PIXEL + MSPARAM_SAFETY_FRAME)

#define ON	1
#define OFF	0

#define DO_NOT_USE_HAL_IRQ_HANDLER

/* send all data/samples/pixel that we recorded, particularly the data
 * that is lying in the data buffer before the valid data. If off we
 * only send the valid samples */
#define DBG_SEND_ALL		ON


#endif /* GLOBAL_CONFIG_H_ */
