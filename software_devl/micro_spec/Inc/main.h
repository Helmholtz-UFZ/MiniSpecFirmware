/**
  ******************************************************************************
  * File Name          : main.h
  * Description        : This file contains the common defines of the application
  ******************************************************************************
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

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC

#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA

#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

#define SENS_ST_Pin GPIO_PIN_10
#define SENS_ST_GPIO_Port GPIOB
#define SENS_CLK_Pin GPIO_PIN_8
#define SENS_CLK_GPIO_Port GPIOA
#define SENS_EOS_Pin GPIO_PIN_9
#define SENS_EOS_GPIO_Port GPIOA
#define SENS_TRG_Pin GPIO_PIN_12
#define SENS_TRG_GPIO_Port GPIOA

#define TEST_PIN_Pin GPIO_PIN_11
#define TEST_PIN_GPIO_Port GPIOA

#define EXTADC1_BUSY_Pin GPIO_PIN_2
#define EXTADC1_BUSY_GPIO_Port GPIOD


/* USER CODE BEGIN Private defines */

// Corresponding ISRs
#define EXTI2_IRQn_BUSY1	EXTI2_IRQn
//#define EXTI9_5_IRQn_EOS	EXTI9_5_IRQn
#define TIM1_UP_TIM16_IRQn_TRG_DONE		TIM1_UP_TIM16_IRQn

// sensor1/adc1:
//	PA pin 0-7
//	PB pin 0-7
#define SENS1_PA_mask	( 0xFF )
#define SENS1_PC_mask	( 0xFF )

// sensor2/adc2:
//	PB pin 0-2 and 4-11
//	PC pin 10-13
#define SENS_PB_mask	( GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 )
#define SENS_PC_mask	( GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 )



/* USER CODE END Private defines */

/**
  * @}
  */ 

/**
  * @}
*/ 

#endif /* __MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
