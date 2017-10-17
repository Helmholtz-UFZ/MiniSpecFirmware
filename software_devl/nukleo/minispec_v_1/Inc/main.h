/**
  ******************************************************************************
  * File Name          : main.h
  * Description        : This file contains the common defines of the application
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2017 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H
  /* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */
#include "global_include.h"
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

#define SENS_ST_Pin GPIO_PIN_10
#define SENS_ST_GPIO_Port GPIOB
#define SENS_CLK_Pin GPIO_PIN_8
#define SENS_CLK_GPIO_Port GPIOA
#define SENS_EOS_Pin GPIO_PIN_9
#define SENS_EOS_GPIO_Port GPIOA
#define TEST_PIN_Pin GPIO_PIN_11
#define TEST_PIN_GPIO_Port GPIOA
#define SENS_TRG_Pin GPIO_PIN_12
#define SENS_TRG_GPIO_Port GPIOA
#define EXTADC1_BUSY_Pin GPIO_PIN_2
#define EXTADC1_BUSY_GPIO_Port GPIOD
#define EXTADC1_BUSY_EXTI_IRQn EXTI2_IRQn

/* USER CODE BEGIN Private defines */

// redefine to prevent strange behavior
#ifdef __packed
#undef __packed
#define __packed __attribute__((__packed__))
#endif /*__packed */

#ifdef __weak
#undef __weak
#define __weak 	__attribute__((weak))
#endif /*__weak */

/** the data format specifies the datatype of the
 * arguments and the data to/from external system.*/
#define DATA_FORMAT_BIN		0

/** @sa DATA_FORMAT_BIN */
#define DATA_FORMAT_ASCII	1

/** header for data in ASCII format
 *  make data look nice on terminal */
#define HEADER_STR	"          0     1     2     3     4     5     6     7     8     9"

/** @sa DATA_FORMAT_BIN */
#define DELIMITER_STR   "-----------------------------------------------------------------"

/**
 * error code enumeration
 */
typedef enum
{
	ERRC_NO_ERROR = 0,
	ERRC_UNKNOWN,
	ERRC_NO_EOS,
	ERRC_EOS_EARLY,
	ERRC_TIMEOUT
} error_code;

/**
 * enum for the defined commands
 */
typedef enum usr_cmd_enum
{
	USR_CMD_UNKNOWN,
	USR_CMD_SET_FORMAT, // 0 - raw/bin or 1 - ascii
	USR_CMD_SET_SENSOR, // UNUSED (for future use) - choose if sens 1 or sens 2
	USR_CMD_WRITE_ITIME,
	USR_CMD_READ_ITIME,
	USR_CMD_GET_DATA, // UNUSED (for future use) save data and return on request
	USR_CMD_SINGLE_MEASURE_START,
	USR_CMD_STREAM_START,
	USR_CMD_STREAM_END,
} usr_cmd_enum_t;


int usr_main( void );
void cpu_enter_sleep_mode( void );
void cpu_enter_run_mode( void );
/* USER CODE END Private defines */

void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)

/**
  * @}
  */ 

/**
  * @}
*/ 

#endif /* __MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
