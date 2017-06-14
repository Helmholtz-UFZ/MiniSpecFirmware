/**
 ******************************************************************************
 * File Name          : TIM.c
 * Description        : This file provides code for the configuration
 *                      of the TIM instances.
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

/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "global_include.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim5;

/* TIM1 init function */
void MX_TIM1_Init( void )
{
	TIM_ClockConfigTypeDef sClockSourceConfig;
	TIM_SlaveConfigTypeDef sSlaveConfig;
	TIM_MasterConfigTypeDef sMasterConfig;
	TIM_IC_InitTypeDef sConfigIC;
	TIM_OC_InitTypeDef sConfigOC;
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 0;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 300 - 1;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	if( HAL_TIM_Base_Init( &htim1 ) != HAL_OK )
	{
		Error_Handler();
	}

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
	sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_INVERTED;
	sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
	sClockSourceConfig.ClockFilter = 0;
	if( HAL_TIM_ConfigClockSource( &htim1, &sClockSourceConfig ) != HAL_OK )
	{
		Error_Handler();
	}

	if( HAL_TIM_IC_Init( &htim1 ) != HAL_OK )
	{
		Error_Handler();
	}

	if( HAL_TIM_PWM_Init( &htim1 ) != HAL_OK )
	{
		Error_Handler();
	}

	if( HAL_TIM_OnePulse_Init( &htim1, TIM_OPMODE_SINGLE ) != HAL_OK )
	{
		Error_Handler();
	}

	sSlaveConfig.SlaveMode = TIM_SLAVEMODE_TRIGGER;
	sSlaveConfig.InputTrigger = TIM_TS_ITR1;
	if( HAL_TIM_SlaveConfigSynchronization( &htim1, &sSlaveConfig ) != HAL_OK )
	{
		Error_Handler();
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if( HAL_TIMEx_MasterConfigSynchronization( &htim1, &sMasterConfig ) != HAL_OK )
	{
		Error_Handler();
	}

	sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
	sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
	sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
	sConfigIC.ICFilter = 0;
	if( HAL_TIM_IC_ConfigChannel( &htim1, &sConfigIC, TIM_CHANNEL_2 ) != HAL_OK )
	{
		Error_Handler();
	}

	sConfigOC.OCMode = TIM_OCMODE_PWM2;
	sConfigOC.Pulse = 5;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if( HAL_TIM_PWM_ConfigChannel( &htim1, &sConfigOC, TIM_CHANNEL_4 ) != HAL_OK )
	{
		Error_Handler();
	}

	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.BreakFilter = 0;
	sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
	sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
	sBreakDeadTimeConfig.Break2Filter = 0;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if( HAL_TIMEx_ConfigBreakDeadTime( &htim1, &sBreakDeadTimeConfig ) != HAL_OK )
	{
		Error_Handler();
	}

	HAL_TIM_MspPostInit( &htim1 );

}
/* TIM2 init function */
void MX_TIM2_Init( void )
{
	TIM_ClockConfigTypeDef sClockSourceConfig;
	TIM_MasterConfigTypeDef sMasterConfig;
	TIM_OC_InitTypeDef sConfigOC;

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 80;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 4;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	if( HAL_TIM_Base_Init( &htim2 ) != HAL_OK )
	{
		Error_Handler();
	}

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if( HAL_TIM_ConfigClockSource( &htim2, &sClockSourceConfig ) != HAL_OK )
	{
		Error_Handler();
	}

	if( HAL_TIM_PWM_Init( &htim2 ) != HAL_OK )
	{
		Error_Handler();
	}

	if( HAL_TIM_OnePulse_Init( &htim2, TIM_OPMODE_SINGLE ) != HAL_OK )
	{
		Error_Handler();
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
	if( HAL_TIMEx_MasterConfigSynchronization( &htim2, &sMasterConfig ) != HAL_OK )
	{
		Error_Handler();
	}

	sConfigOC.OCMode = TIM_OCMODE_PWM2;
	sConfigOC.Pulse = 4;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if( HAL_TIM_PWM_ConfigChannel( &htim2, &sConfigOC, TIM_CHANNEL_3 ) != HAL_OK )
	{
		Error_Handler();
	}

	HAL_TIM_MspPostInit( &htim2 );

}

/* TIM5 init function */
void MX_TIM5_Init( void )
{
	TIM_ClockConfigTypeDef sClockSourceConfig;
	TIM_MasterConfigTypeDef sMasterConfig;

	htim5.Instance = TIM5;
	htim5.Init.Prescaler = 0;
	htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim5.Init.Period = 0;
	htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	if( HAL_TIM_Base_Init( &htim5 ) != HAL_OK )
	{
		Error_Handler();
	}

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if( HAL_TIM_ConfigClockSource( &htim5, &sClockSourceConfig ) != HAL_OK )
	{
		Error_Handler();
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if( HAL_TIMEx_MasterConfigSynchronization( &htim5, &sMasterConfig ) != HAL_OK )
	{
		Error_Handler();
	}

}

void HAL_TIM_Base_MspInit( TIM_HandleTypeDef* tim_baseHandle )
{

	GPIO_InitTypeDef GPIO_InitStruct;
	if( tim_baseHandle->Instance == TIM1 )
	{
		/* USER CODE BEGIN TIM1_MspInit 0 */

		/* USER CODE END TIM1_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_TIM1_CLK_ENABLE()
		;

		/**TIM1 GPIO Configuration
		 PA9     ------> TIM1_CH2
		 PA12     ------> TIM1_ETR
		 */
		GPIO_InitStruct.Pin = SENS_EOS_Pin | SENS_TRG_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
		HAL_GPIO_Init( GPIOA, &GPIO_InitStruct );

		/* USER CODE BEGIN TIM1_MspInit 1 */

		/* USER CODE END TIM1_MspInit 1 */
	}
	else if( tim_baseHandle->Instance == TIM2 )
	{
		/* USER CODE BEGIN TIM2_MspInit 0 */

		/* USER CODE END TIM2_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_TIM2_CLK_ENABLE()
		;
		/* USER CODE BEGIN TIM2_MspInit 1 */

		/* USER CODE END TIM2_MspInit 1 */
	}
	else if( tim_baseHandle->Instance == TIM5 )
	{
		/* USER CODE BEGIN TIM5_MspInit 0 */

		/* USER CODE END TIM5_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_TIM5_CLK_ENABLE()
		;
		/* USER CODE BEGIN TIM5_MspInit 1 */

		/* USER CODE END TIM5_MspInit 1 */
	}
}
void HAL_TIM_MspPostInit( TIM_HandleTypeDef* timHandle )
{

	GPIO_InitTypeDef GPIO_InitStruct;
	if( timHandle->Instance == TIM1 )
	{
		/* USER CODE BEGIN TIM1_MspPostInit 0 */

		/* USER CODE END TIM1_MspPostInit 0 */
		/**TIM1 GPIO Configuration
		 PA11     ------> TIM1_CH4
		 */
		GPIO_InitStruct.Pin = TEST_PIN_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
		HAL_GPIO_Init( TEST_PIN_GPIO_Port, &GPIO_InitStruct );

		/* USER CODE BEGIN TIM1_MspPostInit 1 */

		/* USER CODE END TIM1_MspPostInit 1 */
	}
	else if( timHandle->Instance == TIM2 )
	{
		/* USER CODE BEGIN TIM2_MspPostInit 0 */

		/* USER CODE END TIM2_MspPostInit 0 */

		/**TIM2 GPIO Configuration
		 PB10     ------> TIM2_CH3
		 */
		GPIO_InitStruct.Pin = SENS_ST_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
		HAL_GPIO_Init( SENS_ST_GPIO_Port, &GPIO_InitStruct );

		/* USER CODE BEGIN TIM2_MspPostInit 1 */

		/* USER CODE END TIM2_MspPostInit 1 */
	}

}

void HAL_TIM_Base_MspDeInit( TIM_HandleTypeDef* tim_baseHandle )
{

	if( tim_baseHandle->Instance == TIM1 )
	{
		/* USER CODE BEGIN TIM1_MspDeInit 0 */

		/* USER CODE END TIM1_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_TIM1_CLK_DISABLE();

		/**TIM1 GPIO Configuration
		 PA9     ------> TIM1_CH2
		 PA11     ------> TIM1_CH4
		 PA12     ------> TIM1_ETR
		 */
		HAL_GPIO_DeInit( GPIOA, SENS_EOS_Pin | TEST_PIN_Pin | SENS_TRG_Pin );

		/* Peripheral interrupt Deinit*/
		HAL_NVIC_DisableIRQ( TIM1_UP_TIM16_IRQn );

		/* USER CODE BEGIN TIM1_MspDeInit 1 */

		/* USER CODE END TIM1_MspDeInit 1 */
	}
	else if( tim_baseHandle->Instance == TIM2 )
	{
		/* USER CODE BEGIN TIM2_MspDeInit 0 */

		/* USER CODE END TIM2_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_TIM2_CLK_DISABLE();
		/* USER CODE BEGIN TIM2_MspDeInit 1 */

		/* USER CODE END TIM2_MspDeInit 1 */
	}
	else if( tim_baseHandle->Instance == TIM5 )
	{
		/* USER CODE BEGIN TIM5_MspDeInit 0 */

		/* USER CODE END TIM5_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_TIM5_CLK_DISABLE();
		/* USER CODE BEGIN TIM5_MspDeInit 1 */

		/* USER CODE END TIM5_MspDeInit 1 */
	}
}

void TIM1_Init( void )
{
	// -1 because pwm2 high = (ARR - CCRx) + 1
	__HAL_TIM_SET_AUTORELOAD( &htim1, MSPARAM_CAPTURE_PXL_END - 1 );
	__HAL_TIM_SET_COMPARE( &htim1, TIM_CHANNEL_4, MSPARAM_CAPTURE_PXL_ST );
}

void TIM2_Init( void )
{
	// pwm2 high = (ARR - CCRx) + 1 = ARR
	__HAL_TIM_SET_AUTORELOAD(&htim2, MSPARAM_DEFAULT_INTTIME);
	__HAL_TIM_SET_COMPARE( &htim2, TIM_CHANNEL_3, 1);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
