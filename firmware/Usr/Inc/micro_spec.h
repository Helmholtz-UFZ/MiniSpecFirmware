/*
 * micro_spec.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#ifndef MICRO_SPEC_H_
#define MICRO_SPEC_H_

#include "global_config.h"

/** desired size in words */
#define SENSOR_DATA_BUFFER_MAX_WORDS		(350)

/**word size in bytes*/
#define SENSOR_WORD_SIZE			(2)

/**size in bytes*/
#define SENSOR_DATA_BUFFER_SIZE			((SENSOR_DATA_BUFFER_MAX_WORDS)*(SENSOR_WORD_SIZE))


typedef struct
{
	const uint16_t size; /*		        !< buffer size in bytes  */
	const uint16_t words; /*                !< buffer size in (16bit-)words  */
	uint16_t* base; /*			!< pointer to the start of the buffer  */
	volatile uint16_t volatile *wptr; /*	!< write pointer for the data, points to the next empty location  */
} sensor_buffer_t;

#define CANARYSIZE	32
typedef struct
{
	uint16_t precanary[CANARYSIZE];
	uint16_t memblock[SENSOR_DATA_BUFFER_MAX_WORDS +1];
	uint16_t postcanary[CANARYSIZE];
} canary_memblock_t;

typedef enum
{
	SENS_UNINITIALIZED = 0,
	SENS_INITIALIZED,
	SENS_MEASURE_STARTED,
	SENS_CAPTURE_DATA,
	SENS_EOS_CAPTURED,
	SENS_MEASURE_DONE,

	//errors
	SENS_ERR_EOS_EARLY,
	SENS_ERR_NO_EOS,
	SENS_ERR_TIMEOUT,
} sensor_status_enum_t;

typedef struct
{
	volatile sensor_status_enum_t status;
	volatile sensor_buffer_t *data;
	uint32_t itime;
} sensor_t;

extern sensor_t sens1;

void sensor_init( void );
void sensor_deinit( void );

uint8_t sensor_measure( void );

uint32_t sensor_set_itime( uint32_t int_time );

#endif /* MICRO_SPEC_H_ */
