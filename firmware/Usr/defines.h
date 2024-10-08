/*
 * helper_defines.h
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */
// KEEP THIS ON TOP //
#undef __weak
#undef __packed
#define __weak   __attribute__((weak))
#define __packed __attribute__((__packed__))
// KEEP THIS ON TOP //

#ifndef HELPER_DEFINES_H_
#define HELPER_DEFINES_H_

/**
 * XXX: ATTENTION do not include stuff here
 * This header is included by main.h.
 * Avoid including high level stuff here.
 * Otherwise conflicts arise, as the HAL
 * isn't initialized yet. **/

// for uint_16_t - cast
#include <stdint.h>

#define ON	1
#define OFF	0

#define MAX(A,B)	((A) > (B) ? (A) : (B) )
#define MIN(A,B)	((A) < (B) ? (A) : (B) )

#define BIT0		((uint16_t)0x0001)
#define BIT1     	((uint16_t)0x0002)
#define BIT2     	((uint16_t)0x0004)
#define BIT3     	((uint16_t)0x0008)
#define BIT4     	((uint16_t)0x0010)
#define BIT5     	((uint16_t)0x0020)
#define BIT6     	((uint16_t)0x0040)
#define BIT7     	((uint16_t)0x0080)
#define BIT8     	((uint16_t)0x0100)
#define BIT9     	((uint16_t)0x0200)
#define BIT10    	((uint16_t)0x0400)
#define BIT11     	((uint16_t)0x0800)
#define BIT12     	((uint16_t)0x1000)
#define BIT13     	((uint16_t)0x2000)
#define BIT14     	((uint16_t)0x4000)
#define BIT15     	((uint16_t)0x8000)

#endif /* HELPER_DEFINES_H_ */
