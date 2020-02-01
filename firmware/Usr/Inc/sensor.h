/*
 * micro_spec.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#ifndef MICRO_SPEC_H_
#define MICRO_SPEC_H_

#include <globalconfig.h>

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
	uint32_t* base; /*			!< pointer to the start of the buffer  */
	volatile uint32_t* volatile wptr; /*	!< write pointer for the data, points to the next empty location  */
} sensor_buffer_t;

#define CANARYSIZE	32
typedef struct
{
	uint32_t precanary[CANARYSIZE];
	uint32_t memblock[SENSOR_DATA_BUFFER_MAX_WORDS +1];
	uint32_t postcanary[CANARYSIZE];
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

typedef enum
{
	ERRC_NO_ERROR = 0,
	ERRC_UNKNOWN,
	ERRC_NO_EOS,
	ERRC_EOS_EARLY,
	ERRC_TIMEOUT,
	ERRC_NOT_INITIALIZED,
} sensor_errorcode;


typedef struct
{
	volatile sensor_status_enum_t status;
	volatile sensor_buffer_t *data;
	uint32_t last_itime;
	sensor_errorcode errc;
} sensor_t;

extern sensor_t sens1;

void sensor_init( void );
void sensor_deinit( void );
void sensor_measure(uint32_t itime);

#endif /* MICRO_SPEC_H_ */
