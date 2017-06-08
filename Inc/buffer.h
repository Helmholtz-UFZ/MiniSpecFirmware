/*
 * measure.h
 *
 *  Created on: Apr 24, 2017
 *      Author: Bert Palm
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#define BUFFER_SIZE	500

typedef struct
{
	uint16_t size;
	volatile uint16_t w_idx;
	volatile uint16_t r_idx;
	volatile uint16_t* buf;
} index_buffer_uint16;

#endif /* BUFFER_H_ */
