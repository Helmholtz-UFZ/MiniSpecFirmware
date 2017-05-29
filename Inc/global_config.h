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
#define MSPARAM_PIXEL	80

/** The number of 'unused' TRG pulses before the sensor putting out the
 * video data */
#define MSPARAM_UNUSED_TRG_CNT	89


#define TIM2_HIGH	MSPARAM_PIXEL
#define TIM2_LOW	MSPARAM_UNUSED_TRG_CNT

//#define MSPARAM_TRG_DELAY_CNT	3

/** defines how many timer clock cycles are 1 us. */
#define TIM2_SCALER		80

#endif /* GLOBAL_CONFIG_H_ */
