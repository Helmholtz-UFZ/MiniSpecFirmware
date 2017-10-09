/*
 * usr_tim.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include "tim.h"
#include "global_include.h"

/**
 * Init the tim1 interfaces to our needs. Overwrite
 * some preferences CubeMx made.
 */
void tim1_Init( void )
{
	// -1 because pwm2 high = (ARR - CCRx) + 1
	__HAL_TIM_SET_AUTORELOAD( &htim1, CAPTURE_PXL_END - 1 );
	__HAL_TIM_SET_COMPARE( &htim1, TIM_CHANNEL_4, CAPTURE_PXL_ST );
}

/**
 * Init the tim2 interfaces to our needs. Overwrite
 * some preferences CubeMx made.
 */
void tim2_Init( void )
{
	//enable the channel so the line is pulled down to ensure a low signal
	TIM2->CCER |= TIM_CCER_CC3E;

	// pwm2 high = (ARR - CCRx) + 1 = ARR
	__HAL_TIM_SET_AUTORELOAD( &htim2, DEFAULT_INTEGRATION_TIME );
	__HAL_TIM_SET_COMPARE( &htim2, TIM_CHANNEL_3, 1 );
}

/**
 * Init the tim2 interfaces to our needs. Overwrite
 * some preferences CubeMx made.
 */
void tim5_Init( void )
{
	__HAL_TIM_SET_AUTORELOAD( &htim5, SAFTY_TIMER_DELAY );
}


