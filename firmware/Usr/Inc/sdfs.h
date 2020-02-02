/*
 * sd_card.h
 *
 *  Created on: Feb 19, 2019
 *      Author: Bert Palm
 */

#ifndef INC_SDFS_H_
#define INC_SDFS_H_

#include <globalconfig.h>
#include "fatfs.h"

/* Extend FRESULT */
#define ERR_NODATA 			20
#define BUF_TOOSMALL 		21

FRESULT sd_stat(const TCHAR* path, FILINFO* fno);
FRESULT sd_open(FIL* fp, const TCHAR* path, BYTE mode);
FRESULT sd_close(FIL* fp);
FRESULT sd_open_file_neworappend(FIL* f, char *fname);
uint8_t sd_find_right_filename(uint16_t offset, uint16_t *postfix, char *namebuf, uint16_t size);


#endif /* INC_SDFS_H_ */
