/*
 * micro_spec.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#ifndef MICRO_SPEC_H_
#define MICRO_SPEC_H_

#include "buffer.h"

extern volatile microspec_buffer sens1_buffer;

typedef enum
{
	        MS_INIT,
	        MS_MEASURE_INIT,
	        MS_TIM2_STARTED,
	        MS_TIM1_CC,
	        MS_TIM1_DONE,
	        MS_COUNT_TRG_DONE,
	        MS_READ_DATA,
	        MS_BUFFER_FULL,
	        MS_EOS,
	        MS_POST_PROCESS,
	        MS_DONE,
	        MS_FAIL

} meas_status_t;

extern volatile meas_status_t status;
extern uint32_t integrtion_time;
volatile uint16_t sens_trg_count;


void micro_spec_init( void );
void micro_spec_deinit( void );
void micro_spec_measure_init( void );
uint32_t micro_spec_set_integration_time( uint32_t int_time );
void micro_spec_measure_start( void );
void micro_spec_measure_deinit( void );

#endif /* MICRO_SPEC_H_ */
