/*
 * sd_card.c
 *
 *  Created on: Feb 19, 2019
 *      Author: palmb_ubu
 */

#include "sd_card.h"
#include "fatfs.h"
#include "string.h"

uint8_t workBuffer[_MAX_SS];

/* File system object for SD card logical drive */
//FATFS SDFatFs;
uint8_t sd_mount(void) {
	return f_mount(&SDFatFS, SDPath, 0);
}

uint8_t sd_umount(void) {
	return f_mount(NULL, SDPath, 0);
}

uint8_t sd_format(void) {
	return f_mkfs(SDPath, FM_ANY, 0, workBuffer, sizeof(workBuffer));
}

/* Append strings to file if it exist or create it.*/
uint8_t sd_write_file(char *fname, char *wtxt) {
	FIL *f = &SDFile;
	int8_t res = 0;
	int16_t byteswritten = -1;

	// try create a File
	res = f_open(f, fname, FA_WRITE | FA_CREATE_NEW);
	switch (res) {

	case FR_OK:
		// new file continue
		break;

	case FR_EXIST:
		// if exist open in append
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

	f_close(f);

	return 0;
}
