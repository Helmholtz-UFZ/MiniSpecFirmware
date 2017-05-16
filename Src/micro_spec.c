/*
 * micro_spec.c
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 */

#include "stm32l4xx_hal.h"
#include "buffer.h"
#include <string.h>
#include "micro_spec.h"

volatile meas_status_t status;
volatile index_buffer_uint16 sens1_buffer;
uint32_t integrtion_time;

extern TIM_HandleTypeDef htim2;

static void enable_sensor_clk( void );
static void disable_sensor_clk( void );
static void send_st_signal( uint32_t integrtion_time );
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
	HAL_Delay( 1 );
	status = MS_CLK_ON;
}

/**
 * Deinit the sensor and all related (Hardware) Modules. Call when all (repetitive)
 * measurements are done.
 */
void micro_spec_measure_deinit( void )
{
	// disable ADC and CLK
	HAL_GPIO_WritePin( EXTADC_EN_GPIO_Port, EXTADC_EN_Pin, GPIO_PIN_RESET );
	disable_sensor_clk();
}

/**
 * @brief Set the integration time for the sensor.
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
	send_st_signal( integrtion_time );
	// all further work is done in the ISRs.
	// 1. The Timer ISR enables the ISR for the TRG and disable itself.
	// 2. After 89th TRG pulse the ADC IR is enabled and the TRG disables itself
	// 3. After 288 conversions the ADC IR disables itself, we're done.
	// .. tim IR -> TRG IR -> ADCBUSY IR
	while( status != MS_DONE )
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
static void send_st_signal( uint32_t integrtion_time )
{
	UNUSED( integrtion_time );
	// todo set timer values (intergartion time)
	HAL_TIM_OnePulse_Start( &htim2, TIM_CHANNEL_1 );
	HAL_TIM_PWM_Start_IT( &htim2, TIM_CHANNEL_1 );
	status = MS_ST_SIGNAL;

}

/**
 * TODO
 */
static void post_process_values( void )
{

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

