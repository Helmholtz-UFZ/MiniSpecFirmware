/*
 * cube_checks.h
 *
 *  Created on: Feb 2, 2020
 *      Author: Bert Palm
 */

#ifndef CUBE_CHECKS_H_
#define CUBE_CHECKS_H_


#include "fatfs_platform.h"
#ifndef FATFS_PLATFORM_OK
#error "CubeMx deleted our code in fatfs_platform.c/.h"
#endif

#include "rtc.h"
#ifndef RTC_REINIT_CODE_OK
#error "CubeMx deleted our code in rtc.c/.h"
#endif

#include "main.h"
#ifndef MAIN_CODE_OK
#error "CubeMx deleted our code in main.c/.h"
#endif

#endif /* CUBE_CHECKS_H_ */
