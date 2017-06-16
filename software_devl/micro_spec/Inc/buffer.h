/*
 * measure.h
 *
 *  Created on: Apr 24, 2017
 *      Author: Bert Palm
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#define BUFFER_MAX_IDX		350

/**todo size in bytes*/
#define BUFFER_SIZE		(BUFFER_MAX_IDX *2)

typedef struct
{
	uint16_t bytes;
	volatile uint16_t w_idx;
	volatile uint16_t r_idx;
	volatile uint16_t* buf;
} index_buffer_uint16;

typedef struct
{
	uint16_t size;
	void *base;
} simple_buffer;

#endif /* BUFFER_H_ */
