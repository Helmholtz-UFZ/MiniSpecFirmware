/*
 * sd_card.c
 *
 *  Created on: Feb 19, 2019
 *      Author: palmb_ubu
 */

#include <lib_sd.h>
#include <lib_uart.h>
#include "string.h"
#include "stdio.h"
#include "sdmmc.h"
#include "stdio_usr.h"


uint8_t workBuffer[_MAX_SS];
static void sd_reinit(void);
static uint8_t sd_find_highest_postfix(uint16_t offset, uint16_t *postfix, char *namebuf, uint16_t size);


/** Sometimes the SD card isn't recognized correctly,
 *  than this try to clean up and init all corresponding
 *  modules. Namely the SD Card and the Fat File System. */
static void sd_reinit(void){
	// deinit
	FATFS_UnLinkDriver(SDPath);
	HAL_SD_DeInit(&hsd1);
	// reinit
	MX_SDMMC1_SD_Init();
	MX_FATFS_Init();
	debug("(sd) reinit sd\n");
}

FRESULT sd_mount(void) {
	FRESULT res = f_mount(&SDFatFS, SDPath, 0);
	debug("(sd) MOUNT: %u\n", res);
	return res;
}

FRESULT sd_umount(void) {
	FRESULT res = f_mount(NULL, SDPath, 0);
	debug("(sd) UMOUNT: %u\n", res);
	return res;
}

FRESULT sd_format(void) {
	return f_mkfs(SDPath, FM_ANY, 0, workBuffer, sizeof(workBuffer));
}

FRESULT sd_stat(const TCHAR* path, FILINFO* fno) {
	FRESULT res;
	uint8_t i = 0;
	do {
		res = f_stat(path, fno);
		if (res == FR_DISK_ERR || res == FR_NOT_READY) {
			sd_reinit();
		} else {
			break;
		}
	} while (++i < SD_MAX_REINIT_DRIVER);
	debug("(sd) STAT: %u\n", res);
	return res;
}

FRESULT sd_open(FIL* fp, const TCHAR* path, BYTE mode) {
	FRESULT res;
	uint8_t i = 0;
	do {
		res = f_open(fp, path, mode);
		if (res == FR_DISK_ERR || res == FR_NOT_READY) {
			sd_reinit();
		} else {
			break;
		}
	} while (++i < SD_MAX_REINIT_DRIVER);
	return res;
}

FRESULT sd_close(FIL* fp) {
	FRESULT res = f_close(fp);
	debug("(sd) CLOSE: %u\n", res);
	return res;
}

/**
 * Find the file with the default name (see SD_BASENAME) with the
 * highest postfix number. And return its name.
 * The name is searched linear, starting from offset.
 *
 * The name is returned in the param namebuf and the highest number in postfix.
 * The function return FR_OK(0) if the file exist and no error occurred.
 * If no file with the BASENAME exist, the function return FR_NO_FILE(4).
 *
 * Return FRESULT(0-19) or BUF_TOOSMALL(21)
 */
static uint8_t sd_find_highest_postfix(uint16_t offset, uint16_t *postfix, char *namebuf,
		uint16_t size) {
	FILINFO info;
	uint8_t res;
	int16_t len = -1;
	uint16_t N = offset;
	uint16_t lastN = N;
	memset(&info, 0, sizeof(FILINFO));

	do {
		len = snprintf(namebuf, size, "%s_%u.%s", SD_FILE_BASENAME, N, SD_FILE_EXTENSION);
		res = sd_stat(namebuf, &info);
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
				len = snprintf(namebuf, size, "%s_%u.%s", SD_FILE_BASENAME, N, SD_FILE_EXTENSION);
			}
			break;
		}
	} while (1);

	if (len < 0 || len >= size) {
		res = BUF_TOOSMALL;
	}
	return res;
}

/**
 * Find or generate a filename from the default name (see SD_BASENAME) which
 * 	i)  has the highest postfix number and
 * 	ii)	does't exceed the file size limit (see SD_MAX_FILESIZE)
 *
 * The file corresponding to the returned name must not necessarily exist.
 *
 * Return FRESULT(0-19) or BUF_TOOSMALL(21)
 **/
uint8_t sd_find_right_filename(uint16_t offset, uint16_t *postfix, char *namebuf, uint16_t size) {
	FILINFO info;
	uint8_t res;
	int16_t len = 0;
	memset(&info, 0, sizeof(FILINFO));

	res = sd_find_highest_postfix(offset, postfix, namebuf, size);
	if (res == FR_OK) {
		/* File exist, check file size */
		res = sd_stat(namebuf, &info);
		if (res == FR_OK) {
			if (info.fsize > SD_MAX_FILESIZE) {
				/* Return a fresh new filename */
				(*postfix)++;
				len = snprintf(namebuf, size, "%s_%u.%s", SD_FILE_BASENAME, *postfix,
				SD_FILE_EXTENSION);
			}
		}
	}
	if (res == FR_NO_FILE) {
		res = FR_OK;
	}
	if (len < 0 || len >= size) {
		res = BUF_TOOSMALL;
	}
	if (res > 0) {
		memset(namebuf, 0, size);
	}
	debug("(sd) FIND: %u\n", res);
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
FRESULT sd_open_file_neworappend(FIL* f, char *fname) {
	FRESULT res = 0;
	res = sd_open(f, fname, FA_WRITE | FA_CREATE_NEW);
	if (res == FR_EXIST) {
		res = sd_open(f, fname, FA_WRITE | FA_OPEN_APPEND);
	}
	debug("(sd) OPEN: %u\n", res);
	return res;
}

