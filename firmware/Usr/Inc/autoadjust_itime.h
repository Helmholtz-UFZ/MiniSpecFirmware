/*
 * autoadjust_itime.h
 *
 *  Created on: Jan 26, 2020
 *      Author: Bert Palm
 */

#ifndef INC_AUTOADJUST_ITIME_H_
#define INC_AUTOADJUST_ITIME_H_

#include "global_config.h"

#define UP 1
#define DOWN -1
#define FOUND 0

uint32_t autoadjust_itime(uint32_t lower, uint32_t upper);

#endif /* INC_AUTOADJUST_ITIME_H_ */
