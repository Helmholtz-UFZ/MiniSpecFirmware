/*
 * micro_spec.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#ifndef MICRO_SPEC_H_
#define MICRO_SPEC_H_


#define MICROSPEC_DATA_BUFFER_MAX_WORDS		(350)

/**word size in bytes*/
#define MICROSPEC_WORD_SIZE			(2)

/**size in bytes*/
#define MICROSPEC_DATA_BUFFER_SIZE		(MICROSPEC_DATA_BUFFER_MAX_WORDS*MICROSPEC_WORD_SIZE)

typedef struct
{
	const uint16_t size2; /*!< buffer size in bytes  */
	const uint16_t words; /*!< buffer size in (16bit-)words  */
	uint16_t* base;
	volatile uint16_t volatile *wptr;
	volatile uint16_t last_valid;
} microspec_buffer;


typedef enum
{
	MS_UNINITIALIZED = 0,
	MS_INITIALIZED,
	MS_MEASUREMENT_READY,
	MS_MEASUREMENT_STARTED,
	MS_MEASUREMENT_ONGOING_TIM1_UP,
	MS_MEASUREMENT_ONGOING_TIM1_CC,
	MS_MEASUREMENT_DONE
} microspec_status_enum_t;

typedef struct
{
	volatile microspec_status_enum_t status;
	volatile microspec_buffer *data;
	uint32_t integrtion_time;

} microspec_t;

extern microspec_t hms1;

void micro_spec_init( void );
void micro_spec_deinit( void );

void micro_spec_measure_init( void );
void micro_spec_measure_start( void );
void micro_spec_wait_for_measurement_done( void );

uint32_t micro_spec_set_integration_time( uint32_t int_time );

#endif /* MICRO_SPEC_H_ */
