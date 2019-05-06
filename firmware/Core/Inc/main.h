/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "global_config.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Button_NucleoBoard_Pin GPIO_PIN_13
#define Button_NucleoBoard_GPIO_Port GPIOC
#define D11_Pin GPIO_PIN_0
#define D11_GPIO_Port GPIOC
#define D9_Pin GPIO_PIN_1
#define D9_GPIO_Port GPIOC
#define D8_Pin GPIO_PIN_2
#define D8_GPIO_Port GPIOC
#define D10_Pin GPIO_PIN_3
#define D10_GPIO_Port GPIOC
#define D0_Pin GPIO_PIN_0
#define D0_GPIO_Port GPIOA
#define D1_Pin GPIO_PIN_1
#define D1_GPIO_Port GPIOA
#define DIP_1_Pin GPIO_PIN_2
#define DIP_1_GPIO_Port GPIOA
#define DIP_2_Pin GPIO_PIN_3
#define DIP_2_GPIO_Port GPIOA
#define D4_Pin GPIO_PIN_4
#define D4_GPIO_Port GPIOA
#define D12_Pin GPIO_PIN_5
#define D12_GPIO_Port GPIOA
#define D7_Pin GPIO_PIN_6
#define D7_GPIO_Port GPIOA
#define D6_Pin GPIO_PIN_7
#define D6_GPIO_Port GPIOA
#define D5_Pin GPIO_PIN_4
#define D5_GPIO_Port GPIOC
#define D13_Pin GPIO_PIN_5
#define D13_GPIO_Port GPIOC
#define TIM3CH3_PROBE_1_Pin GPIO_PIN_0
#define TIM3CH3_PROBE_1_GPIO_Port GPIOB
#define TIM3CH4_SENSCLK_Pin GPIO_PIN_1
#define TIM3CH4_SENSCLK_GPIO_Port GPIOB
#define EXTADC_BUSY_Pin GPIO_PIN_2
#define EXTADC_BUSY_GPIO_Port GPIOB
#define EXTADC_BUSY_EXTI_IRQn EXTI2_IRQn
#define TIM2CH3_SENSST_Pin GPIO_PIN_10
#define TIM2CH3_SENSST_GPIO_Port GPIOB
#define POWER5V_SWITCH_ENBL_Pin GPIO_PIN_12
#define POWER5V_SWITCH_ENBL_GPIO_Port GPIOB
#define W5500_RST_Pin GPIO_PIN_13
#define W5500_RST_GPIO_Port GPIOB
#define W5500_SCS_Pin GPIO_PIN_14
#define W5500_SCS_GPIO_Port GPIOB
#define DIP_3_Pin GPIO_PIN_15
#define DIP_3_GPIO_Port GPIOB
#define D14_Pin GPIO_PIN_6
#define D14_GPIO_Port GPIOC
#define D15_Pin GPIO_PIN_7
#define D15_GPIO_Port GPIOC
#define SD_DETECT_IN_Pin GPIO_PIN_9
#define SD_DETECT_IN_GPIO_Port GPIOC
#define D2_Pin GPIO_PIN_8
#define D2_GPIO_Port GPIOA
#define TIM1CH2_SENSEOS_Pin GPIO_PIN_9
#define TIM1CH2_SENSEOS_GPIO_Port GPIOA
#define D3_Pin GPIO_PIN_10
#define D3_GPIO_Port GPIOA
#define TIM1CH4_PROBE_0_Pin GPIO_PIN_11
#define TIM1CH4_PROBE_0_GPIO_Port GPIOA
#define TIM1ETR_SENSTRG_Pin GPIO_PIN_12
#define TIM1ETR_SENSTRG_GPIO_Port GPIOA
#define W5500_INT_Pin GPIO_PIN_15
#define W5500_INT_GPIO_Port GPIOA
#define SD_DETECT_OUT_Pin GPIO_PIN_10
#define SD_DETECT_OUT_GPIO_Port GPIOC
#define RESERVED_Pin GPIO_PIN_11
#define RESERVED_GPIO_Port GPIOC
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
