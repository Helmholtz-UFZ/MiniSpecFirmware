/*
 * global_config.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#ifndef GLOBAL_CONFIG_H_
#define GLOBAL_CONFIG_H_

/** The delay between function call and ST goes high.
 *  This only is used by the timer, to ensure that the initial
 *  value is not zero*/
#define PRE_ST_DELAY		1

/** The minimal possible integration time, limited by the sensor.
 * 	Hightime of ST-signal + 48 Clk cycles
 * 	@1Mhz: 1.2us + 48 * 1/10^6 = 49.2 us*/
#define MIN_INTERGATION_TIME	50

/** The number of pixels the sensor provide. This defines also the number of
 * TRG pulses between MSPARAM_UNUSED_TRG_CNT and the rising edge of SENS_EOS. */
#define MSPARAM_PIXEL	(288)

/** The number of 'unused' TRG pulses before the sensor putting out the
 * video data */
#define MSPARAM_UNUSED_TRG_CNT	(88)

// captured pixel 85,86,87,88 are not valid
#define MSPARAM_CAPTURE_PXL_ST	(85)

//288 + 88 = 376 => 4 to much
#define MSPARAM_CAPTURE_PXL_END	(380)

#define MSPARAM_DEFAULT_INTTIME	(1100)

#define TIM2_HIGH	(MSPARAM_PIXEL+20)
#define TIM2_LOW	(MSPARAM_UNUSED_TRG_CNT)

//#define MSPARAM_TRG_DELAY_CNT	3

/** defines how many timer clock cycles are 1 us. */
#define TIM2_SCALER		80

#define EXTERNAL_COMMUNICATION_UART_BUFFER_SIZE	256

#define CONTINIOUS_MODE		OFF


#define ON	1
#define OFF	0

#define DO_NOT_USE_HAL_IRQ_HANDLER

/** If ON: connect pysically SENS_CLK with ADC1_BUSY, then everything will work,
 * except EOS (not needed). The data is generated random or static.*/
#define DBG_SIMULATE_SENSOR	OFF

#define DBG_SIMULATE_ALL	OFF


#endif /* GLOBAL_CONFIG_H_ */
