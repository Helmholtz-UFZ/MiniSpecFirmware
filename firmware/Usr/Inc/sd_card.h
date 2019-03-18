/*
 * sd_card.h
 *
 *  Created on: Feb 19, 2019
 *      Author: palmb_ubu
 */

#ifndef INC_SD_CARD_H_
#define INC_SD_CARD_H_

#include "global_config.h"
#include "fatfs.h"

#define ERR_NODATA 			20
#define BUF_TOOSMALL 		21

uint8_t sd_format(void);
uint8_t sd_mount(void);
uint8_t sd_umount(void);
uint8_t sd_write_file(char *fname, char *wtxt);
uint8_t sd_open_file_neworappend(FIL* f, char *fname);


#endif /* INC_SD_CARD_H_ */
