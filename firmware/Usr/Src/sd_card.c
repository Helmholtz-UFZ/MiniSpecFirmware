/*
 * sd_card.c
 *
 *  Created on: Feb 19, 2019
 *      Author: palmb_ubu
 */

#include "sd_card.h"
#include "string.h"

uint8_t workBuffer[_MAX_SS];
static FRESULT sd_find_highest_postfix(uint offset, uint *postfix, char *namebuf, uint size);

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
	memset(f, 0, sizeof(FIL));

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
 * Find the file with the default name (see SD_BASENAME) with the
 * highest postfix number. And return its name.
 * The name is searched linear, starting from offset.
 *
 * The name is returned in the param namebuf and the highest number in postfix.
 * The function return FR_OK(0) if the file exist and no error occurred.
 * If no file with the BASENAME exist, the function return FR_NO_FILE(4).
 */
static FRESULT sd_find_highest_postfix(uint offset, uint *postfix, char *namebuf, uint size) {
	FILINFO info;
	FRESULT res;
	uint N = offset;
	uint lastN = N;
	memset(&info, 0, sizeof(FILINFO));

	do {
		snprintf(namebuf, size, "%s_%u.%s", SD_FILE_BASENAME, N, SD_FILE_EXTENSION);
		res = f_stat(namebuf, &info);
		if (res == FR_OK) {
			/* found file, so we check the next one */
			lastN = N++;
		} else {
			if (res == FR_NO_FILE) {
				if (N > 0) {
					N = lastN;
					res = FR_OK;
				}
				*postfix = N;
				snprintf(namebuf, size, "%s_%u.%s", SD_FILE_BASENAME, N, SD_FILE_EXTENSION);
			}
			break;
		}
	} while (1);

	return res;
}


/**
 * Find or generate a filename from the default name (see SD_BASENAME) which
 * 	i)  has the highest postfix number and
 * 	ii)	does't exceed the file size limit (see SD_MAX_FILESIZE)
 *
 * 	The file corresponding to the returned name must not necessarily exist.
 **/
FRESULT sd_find_right_filename(uint offset, uint *postfix, char *namebuf, uint size) {
	FRESULT res;
	FILINFO info;
	memset(&info, 0, sizeof(FILINFO));

	res = sd_find_highest_postfix(offset, postfix, namebuf, size);
	if (res == FR_OK) {
		/* File exist, check file size */
		res = f_stat(namebuf, &info);
		if (res == FR_OK) {
			if (info.fsize > SD_MAX_FILESIZE) {
				/* Return a fresh new filename */
				(*postfix)++;
				snprintf(namebuf, size, "%s_%u.%s", SD_FILE_BASENAME, *postfix, SD_FILE_EXTENSION);
			}
		}
	}
	return res;
}

/**
 * Open and return a file in append mode if it exist, or
 * create and return it, if not.
 *
 * param *f: A File object see SDFile.
 *
 * Note: The File object must be initialisized with zero
 * or unexpected errors will occur. If file object is created
 * on file/header level it is init by default.
 */
uint8_t sd_open_file_neworappend(FIL* f, char *fname) {
	int8_t res = 0;
	res = f_open(f, fname, FA_WRITE | FA_CREATE_NEW);
	if (res == FR_EXIST) {
		res = f_open(f, fname, FA_WRITE | FA_OPEN_APPEND);
	}
	return res;
}
