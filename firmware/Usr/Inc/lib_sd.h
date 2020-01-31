/*
 * sd_card.h
 *
 *  Created on: Feb 19, 2019
 *      Author: Bert Palm
 */

#ifndef INC_LIB_SD_H_
#define INC_LIB_SD_H_

#include "global_config.h"
#include "fatfs.h"

/* Extend FRESULT */
#define ERR_NODATA 			20
#define BUF_TOOSMALL 		21

FRESULT sd_format(void);
FRESULT sd_mount(void);
FRESULT sd_umount(void);
FRESULT sd_stat(const TCHAR* path, FILINFO* fno);
FRESULT sd_open(FIL* fp, const TCHAR* path, BYTE mode);
FRESULT sd_close(FIL* fp);
FRESULT sd_open_file_neworappend(FIL* f, char *fname);
uint8_t sd_find_right_filename(uint16_t offset, uint16_t *postfix, char *namebuf, uint16_t size);


#endif /* INC_LIB_SD_H_ */
