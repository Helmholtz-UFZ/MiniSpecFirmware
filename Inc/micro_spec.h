/*
 * micro_spec.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#ifndef MICRO_SPEC_H_
#define MICRO_SPEC_H_

/** The minimal possible integration time, limited by the sensor.
 * 	Hightime of ST-signal + 48 Clk cycles
 * 	@1Mhz: 1.2us + 48 * 1/10^6 = 49.2 us
 */
#define MIN_INTERGATION_TIME	50

typedef enum
{
	        MS_CLK_ON,
	        MS_ST_SIGNAL,
	        MS_COUNT_TRG,
	        MS_ADC_,
	        MS_DONE
} meas_status_t;

void micro_spec_init( void );
void micro_spec_deinit( void );
void micro_spec_measure_init( void );
uint32_t micro_spec_set_integration_time( uint32_t int_time );
void micro_spec_measure_start( void );
void micro_spec_measure_deinit( void );

#endif /* MICRO_SPEC_H_ */
