/*
 * usr_tim.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include "tim.h"
#include "global_include.h"

void tim1_Init( void )
{
	// -1 because pwm2 high = (ARR - CCRx) + 1
	__HAL_TIM_SET_AUTORELOAD( &htim1, MSPARAM_CAPTURE_PXL_END - 1 );
	__HAL_TIM_SET_COMPARE( &htim1, TIM_CHANNEL_4, MSPARAM_CAPTURE_PXL_ST );
}

void tim2_Init( void )
{
	//enable the channel so the line is pulled down to a
	// ceratin state
	TIM2->CCER |= TIM_CCER_CC3E;

	// pwm2 high = (ARR - CCRx) + 1 = ARR
	__HAL_TIM_SET_AUTORELOAD( &htim2, MSPARAM_DEFAULT_INTTIME );
	__HAL_TIM_SET_COMPARE( &htim2, TIM_CHANNEL_3, 1 );
}

