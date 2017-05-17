/*
 * micro_spec.c
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#include "stm32l4xx_hal.h"
#include <string.h>
#include "micro_spec.h"
#include "global_include.h"
#include "tim.h"

volatile index_buffer_uint16 sens1_buffer;
static uint32_t integrtion_time;

static void enable_sensor_clk( void );
static void disable_sensor_clk( void );
static void send_st_signal( void );
static void post_process_values( void );

/**
 * Init the sensor and all necessary HW.
 */
void micro_spec_init( void )
{
	integrtion_time = 1000;
	uint16_t sz = 300;
	uint16_t x[sz];
	memset( x, 0, sizeof(uint16_t) * sz );

	sens1_buffer.buf = x;
	sens1_buffer.size = sz;
	sens1_buffer.r_idx = 0;
	sens1_buffer.w_idx = 0;
}

/**
 * Init the spectrometer for a single measurement.
 */
void micro_spec_measure_init( void )
{
	enable_sensor_clk();
	sens1_buffer.w_idx = 0; // HACK
	sens_trg_count = 0;
	status = MS_CLK_ON;
}

/**
 * Deinit the sensor and all related (Hardware) Modules. Call when all (repetitive)
 * measurements are done.
 */
void micro_spec_measure_deinit( void )
{
	disable_sensor_clk();
}

/**
 * @brief 	Set the integration time for the sensor.
 *
 * The minimum is defined by MIN_INTERGATION_TIME ( 50 us )
 * The maximum is (UINT32_MAX / TIM2_SCALER) - MIN_INTERGATION_TIME ( ~53 seconds )
 *
 * @param int_time	The integration time in us
 * @return The integration time value set
 */
uint32_t micro_spec_set_integration_time( uint32_t int_time )
{
	if( int_time < MIN_INTERGATION_TIME )
	{
		integrtion_time = MIN_INTERGATION_TIME;
	}
	else
	{
		integrtion_time = int_time;
	}

	return integrtion_time;
}

/**
 * Start a single measurement.
 */
void micro_spec_measure_start( void )
{
	send_st_signal();
	// all further work is done in the ISRs.
	// 1. The Timer ISR enables the ISR for the TRG and disable itself.
	// 2. After 89th TRG pulse the ADC IR is enabled and the TRG disables itself
	// 3. After 288 conversions the ADC IR disables itself, we're done.
	// .. tim IR -> TRG IR -> ADCBUSY IR
	while( status != MS_READ_ADC_DONE )
	{
		// busy waiting
	}
	post_process_values();
}

/**
 * Set the ST-signal high for a defined time, then back low again.
 * The ST-signal act as the start pulse for the sensor and it is
 * also used to determine the integration time that is used in the
 * following (just started) measurement.
 */
static void send_st_signal( void )
{
	uint32_t int_time;

	int_time = MAX( integrtion_time, MIN_INTERGATION_TIME );
	int_time -= 48;

	// set the period // todo check uint_32 limits
	TIM2->ARR = (int_time * TIM2_SCALER) + PRE_ST_DELAY;

	// Disable the Channel 1: Reset the CC1E Bit, it is re-enabled
	// by calling HAL_TIM_xxx_Start()
	TIM2->CCER &= ~TIM_CCER_CC1E;

	// set the delay to ensure that the CCR1 is not zero
	TIM2->CCR1 = PRE_ST_DELAY;

	// update shadow regs
	TIM2->EGR = TIM_EGR_UG;

	// clear update flag
	TIM2->SR &= ~TIM_SR_UIF;

	status = MS_ST_SIGNAL_TIM_STARTED;
	NVIC_EnableIRQ(TIM2_IRQn);
	HAL_TIM_OnePulse_Start( &htim2, TIM_CHANNEL_1 );
	HAL_TIM_Base_Start_IT(&htim2);
//	HAL_TIM_PWM_Start_IT( &htim2, TIM_CHANNEL_1 );
}

/**
 * TODO post_process_values()
 */
static void post_process_values( void )
{
	status = MS_POST_PROCESS;
	// ...
	status = MS_DONE;
}

/**
 * Enables the clock signal for the sensor.
 * This gated through the sensor (with a small delay ~43ns)
 * and given back as TRG-signal from the sensor to the ADC-
 * trigger input.
 */
void enable_sensor_clk( void )
{
	GPIO_InitTypeDef GPIO_InitStruct;
	/*Configure GPIO pin for the Sensors CLK
	 * STM32 --> SENS1 & SENS2*/
	GPIO_InitStruct.Pin = SENS_CLK_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
	HAL_GPIO_Init( SENS_CLK_GPIO_Port, &GPIO_InitStruct );
}
/**
 * Disable the sensor clock. When the CLK-signal is deactivated,
 * also the TRG-signal is deactivated. @sa enable_sensor_clk()
 */
void disable_sensor_clk( void )
{
	GPIO_InitTypeDef GPIO_InitStruct;
	/*Configure GPIO pin for the Sensors CLK
	 * STM32 --> SENS1 & SENS2*/
	GPIO_InitStruct.Pin = SENS_CLK_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init( SENS_CLK_GPIO_Port, &GPIO_InitStruct );
}

