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
	MS_UNINITIALIZED = 0,
	MS_INITIALIZED,
	MS_MEASUREMENT_READY,
	MS_MEASUREMENT_STARTED,
	MS_MEASUREMENT_ONGOING_TIM1_UP,
	MS_MEASUREMENT_ONGOING_TIM1_CC,
	MS_MEASUREMENT_DONE
} meas_status_t;

extern volatile meas_status_t status;
extern uint32_t integrtion_time;
volatile uint16_t sens_trg_count;

void micro_spec_init( void );
void micro_spec_deinit( void );
void micro_spec_measure_init( void );
uint32_t micro_spec_set_integration_time( uint32_t int_time );
void micro_spec_measure_start( void );
void micro_spec_wait_for_measurement_done( void );
void micro_spec_measure_deinit( void );

#endif /* MICRO_SPEC_H_ */
