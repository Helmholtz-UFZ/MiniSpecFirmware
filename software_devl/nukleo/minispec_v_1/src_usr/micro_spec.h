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
	const uint16_t size; /*		        !< buffer size in bytes  */
	const uint16_t words; /*                !< buffer size in (16bit-)words  */
	uint16_t* base; /*			!< pointer to the start of the buffer  */
	volatile uint16_t volatile *wptr; /*	!< write pointer for the data, points to the next empty location  */
} microspec_buffer_t;


typedef enum
{
	MS_UNINITIALIZED = 0,
	MS_INITIALIZED,
	MS_MEASURE_STARTED,
	MS_MEASUREMENT_ONGOING_TIM1_CC, // todo rename CAPTURE_DATA
	MS_EOS_CAPTURED,
	MS_MEASURE_DONE,

	//errors
	MS_ERR_EOS_EARLY,
	MS_ERR_NO_EOS,
	MS_ERR_TIMEOUT,
} microspec_status_enum_t;

typedef struct
{
	volatile microspec_status_enum_t status;
	volatile microspec_buffer_t *data;
	uint32_t integrtion_time;

} microspec_t;

extern microspec_t hms1;

void micro_spec_init( void );
void micro_spec_deinit( void );

uint8_t micro_spec_measure_start( void );

uint32_t micro_spec_set_integration_time( uint32_t int_time );

#endif /* MICRO_SPEC_H_ */
