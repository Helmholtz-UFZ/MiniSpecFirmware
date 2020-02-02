/*
 *
 *  Created on: Feb 1, 2020
 *      Author: Bert Palm
 */

#ifndef INC_PPRINT_H_
#define INC_PPRINT_H_

#include "sysrc.h"

/** header for data in ASCII format
 *  make data look nice on terminal */
#define HEADER_STR	"          0     1     2     3     4     5     6     7     8     9"

/** @sa DATA_FORMAT_BIN */
#define DELIMITER_STR   "-----------------------------------------------------------------"


void ok(void);
void argerr(void);
void print_config(runtime_config_t *rc, char *name);
void send_data(void);
void send_itime(void);
void send_rtc_time(void);


#endif /* INC_PPRINT_H_ */
