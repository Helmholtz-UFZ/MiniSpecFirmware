/*
 * sd_card.c
 *
 *  Created on: Feb 19, 2019
 *      Author: palmb_ubu
 */

#include "sd_card.h"
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
	memset(f,0,sizeof(FIL));

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

/**
 * Create a file and return it if it not exist, otherwise
 * return the file in append mode.
 * param *f: A File object see SDFile.
 *
 * Note: The File object must be initialisized with zero
 * or unexpected errors will occur. If file object is created
 * on file/header level it is init by default.
 */
uint8_t sd_open_file_neworappend(FIL* f, char *fname) {
	int8_t res = 0;
	res = f_open(f, fname, FA_WRITE | FA_CREATE_NEW);
	if (res ==  FR_EXIST){
		res = f_open(f, fname, FA_WRITE | FA_OPEN_APPEND);
	}
	return res;
}
