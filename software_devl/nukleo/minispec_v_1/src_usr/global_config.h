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
#define MAX_INTERGATION_TIME		(1000000)


/* The number of pixels the sensor provide.*/
#define MSPARAM_PIXEL			(288)

#define PRE_VALID			(88)
#define TRG_TO_EOS			((PRE_VALID) + (MSPARAM_PIXEL))

#define MSPARAM_DEFAULT_ITIME		(1100)
#define MSPARAM_SAFETY_FRAME		(5)
/**
 * To test, watch the TEST signal on pin PA11 with the oszilloscope and
 * set MSPARAM_SAFETY_FRAME to zero, than the rising edge should be at
 * the first valid VID pixel (synchron with rising edge of TRG) and the
 * falling edge should be on the rising edge of TRG right *after* the last
 * valid pixel.
 *
 * It helps to set the integration time very high, to see the valid pixel
 * range on the VID-signal-line.
 * todo rm MSPARAM_
 * */
#define MSPARAM_CAPTURE_PXL_ST		((PRE_VALID+1) - (MSPARAM_SAFETY_FRAME))
#define MSPARAM_CAPTURE_PXL_END		((PRE_VALID+1) + (MSPARAM_PIXEL) + (MSPARAM_SAFETY_FRAME) )

/**
 * in numbers of CLK
 */
#define ITIME_CORRECTION		(48)

/**
 * The time in ns after the safty timer throw
 * an IR and stop a failed measurement.
 */
#define SAFTY_TIMER_DELAY		(500)

#define ON	1
#define OFF	0

#define DO_NOT_USE_HAL_IRQ_HANDLER

/* send all data/samples/pixel that we recorded, particularly the data
 * that is lying in the data buffer before the valid data. If off we
 * only send the valid samples (for DATA_FORMAT_ASCII only)*/
#define DBG_SEND_ALL		ON


#endif /* GLOBAL_CONFIG_H_ */
