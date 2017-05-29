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
volatile meas_status_t status;
static uint32_t integrtion_time;

static void enable_sensor_clk( void );
static void disable_sensor_clk( void );
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
 * @brief 	Set the integration time in us for the sensor.
 *
 * The minimum is defined by MIN_INTERGATION_TIME ( 50 us )
 * The maximum is (UINT32_MAX / TIM2_SCALER) - PRE_ST_DELAY ( ~53 seconds )
 *
 * @param int_time	The integration time in us
 * @return The integration time value set
 */
uint32_t micro_spec_set_integration_time( uint32_t int_time )
{
	uint64_t max = (int_time * TIM2_SCALER) + PRE_ST_DELAY;

	if( int_time < MIN_INTERGATION_TIME )
	{
		integrtion_time = MIN_INTERGATION_TIME;
	}
	else if( max > UINT32_MAX )
	{
		integrtion_time = (UINT32_MAX - PRE_ST_DELAY) / TIM2_SCALER;
	}
	else
	{
		integrtion_time = int_time;
	}

	return integrtion_time;
}

/**
 * @ brief Start a single measurement.
 *
 *	All further work is done in by the timers TIM1 and TIM2 and in the GPIO ISR.
 *	1. TIM1 generating the start signal (ST) for the sensor
 *	2. TIM2 count the trigger pulses (TRG) from the sensor directly after
 *	ST goes low and throw an IR when MSPARAM_TRG_CNT many pulses occurred.
 *	TIM2 is started from the update event (UEV) of TIM1.
 *	3. TIM2 enables the GPIO IR on the EXTADCx_BUSY Pin(s). So if the external
 *	ADC signals "ready to read" we capture 16 bits on 16 input lines and save.
 *	4. After MSPARAM_PIXEL conversions the external ADC we disables the IR on the
 *	EXTADCx_BUSY-PIN and we're done.
 *	(5. we post process the captured data, if necessary.)
 */
void micro_spec_measure_start( void )
{
	uint32_t int_time_cnt;

	// 48 clock-cycles are added to ST-signal-"high" resulting in integrationtime
	// (see c12880ma_kacc1226e.pdf)
	const uint8_t clk_cycl = 48;

	int_time_cnt = MAX( integrtion_time, MIN_INTERGATION_TIME );
	int_time_cnt -= clk_cycl;
	int_time_cnt = (int_time_cnt * TIM2_SCALER) + PRE_ST_DELAY;

	__HAL_TIM_SET_AUTORELOAD( &htim1, 0xFFFF ); //hack
	status = MS_ST_SIGNAL_TIM_STARTED;

	// enable TIM channels
	TIM_CCxChannelCmd( TIM1, TIM_CHANNEL_2, TIM_CCx_ENABLE );
	TIM_CCxChannelCmd( TIM2, TIM_CHANNEL_3, TIM_CCx_ENABLE );

	// enable TIM IRs
//	__HAL_TIM_CLEAR_IT( &htim1, TIM_IT_UPDATE );
//	__HAL_TIM_ENABLE_IT( &htim1, TIM_IT_UPDATE );

	__HAL_TIM_CLEAR_IT( &htim2, TIM_IT_UPDATE );
	__HAL_TIM_ENABLE_IT( &htim2, TIM_IT_UPDATE );

//	__HAL_TIM_CLEAR_IT( &htim2, TIM_IT_CC3 );
//	__HAL_TIM_ENABLE_IT( &htim2, TIM_IT_CC3 );

	// start TIM1
	__HAL_TIM_MOE_ENABLE( &htim1 );
	__HAL_TIM_ENABLE( &htim1 );

	while( status != MS_READ_ADC_DONE )
	{
		// busy waiting
	}
	post_process_values();
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
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
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
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init( SENS_CLK_GPIO_Port, &GPIO_InitStruct );
}

