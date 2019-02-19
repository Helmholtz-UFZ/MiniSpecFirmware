/*
 * sd_card.c
 *
 *  Created on: Feb 19, 2019
 *      Author: palmb_ubu
 */

#include "sd_card.h"
#include "fatfs.h"
#include "string.h"

FIL _file;
/* File system object for SD card logical drive */
FATFS SDFatFs;

uint8_t sd_mount(void) {
	int8_t res = 0;
	FATFS *fs = &SDFatFS;
	res = f_mount(&SDFatFs, (TCHAR const*) SDPath, 0);
	if (res != FR_OK) {
		return res;
	}
	return 0;
}

uint8_t sd_write_file(char *fname, char *wtxt) {
	FIL *f = &_file;
	int8_t res = 0;
	int16_t byteswritten = -1;

	memset(f, 0, sizeof(FIL));
	res = f_open(&_file, "XXX.TXT", FA_WRITE | FA_CREATE_ALWAYS);
	switch (res) {
	case FR_OK:
		// new file continue
		break;
	case FR_EXIST:
		// open in append
		memset(f, 0, sizeof(FIL));
		res = f_open(f, fname, FA_WRITE | FA_OPEN_APPEND);
		if (res != FR_OK) {
			return res;
		}
		break;
	default:
		return res;
	}

	res = f_write(f, wtxt, (unsigned int) strlen(wtxt), (void *) &byteswritten);
	if (res != FR_OK) {
		return res;
	}
	if (byteswritten <= 0) {
		return ERR_NODATA;
	}

	return 0;
}

