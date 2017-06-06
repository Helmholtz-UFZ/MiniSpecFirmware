/*
 * micro_spec.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#ifndef MICRO_SPEC_H_
#define MICRO_SPEC_H_

#include "buffer.h"

extern volatile index_buffer_uint16 sens1_buffer;

typedef enum
{
	        MS_CLK_ON,
	        MS_ST_SIGNAL_TIM_STARTED,
	        MS_ST_SIGNAL_TIM_DONE,
	        MS_COUNT_TRG,
	        MS_COUNT_TRG_DONE,
	        MS_READ_ADC,
	        MS_READ_ADC_DONE,
	        MS_POST_PROCESS,
	        MS_DONE,
	        MS_FAIL

} meas_status_t;

extern volatile meas_status_t status;
volatile uint16_t sens_trg_count;

void micro_spec_init( void );
void micro_spec_deinit( void );
void micro_spec_measure_init( void );
uint32_t micro_spec_set_integration_time( uint32_t int_time );
void micro_spec_measure_start( void );
void micro_spec_measure_deinit( void );

#endif /* MICRO_SPEC_H_ */
