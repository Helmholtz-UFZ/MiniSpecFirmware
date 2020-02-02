/*
 * measurements.h
 *
 *  Created on: Feb 2, 2020
 *      Author: Bert Palm
 */

#ifndef INC_MEASUREMENTS_H_
#define INC_MEASUREMENTS_H_

#include <stdbool.h>
#include <stdint.h>

void measure(uint32_t itime);
void multimeasure(bool to_sd);

#endif /* INC_MEASUREMENTS_H_ */
