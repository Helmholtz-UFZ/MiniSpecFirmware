/*
 *
 *  Created on: Feb 1, 2020
 *      Author: Bert Palm
 */

#ifndef INC_PPRINT_H_
#define INC_PPRINT_H_

#include <globalconfig.h>
#include <mainloop.h>

void ok(void);
void argerr(void);
void print_config(runtime_config_t *rc, char *name);
void send_data(void);
void send_itime(void);
void send_rtc_time(void);


#endif /* INC_PPRINT_H_ */
