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

volatile microspec_buffer sens1_buffer;
volatile meas_status_t status;
uint32_t integrtion_time = 0;
static uint16_t mem_block0[BUFFER_MAX_IDX + 1];

static void enable_sensor_clk( void );
static void disable_sensor_clk( void );
static void post_process_values( void );

/**
 *@brief Init all internal data structs and buffer for the sensor.
 */
void micro_spec_init( void )
{
	// Enable TIM1 IRs
	NVIC_ClearPendingIRQ( TIM1_UP_TIM16_IRQn );
	NVIC_ClearPendingIRQ( TIM1_CC_IRQn );
	NVIC_EnableIRQ( TIM1_UP_TIM16_IRQn );
	NVIC_EnableIRQ( TIM1_CC_IRQn );

	integrtion_time = (integrtion_time == 0) ? MSPARAM_DEFAULT_INTTIME : integrtion_time;
	sens1_buffer.buf = mem_block0;
	sens1_buffer.bytes = BUFFER_SIZE;
	sens1_buffer.w_idx = 0;

	enable_sensor_clk();
	HAL_Delay( 1 );
	status = MS_INITIALIZED;
}

void micro_spec_deinit( void )
{
	HAL_Delay( 1 );
	disable_sensor_clk();

	// Enable TIM1 IRs
	NVIC_DisableIRQ( TIM1_UP_TIM16_IRQn );
	NVIC_DisableIRQ( TIM1_CC_IRQn );
	status = MS_UNINITIALIZED;
}

/**
 * Init the spectrometer for a single measurement.
 */
void micro_spec_measure_init( void )
{
	memset( (microspec_buffer*) sens1_buffer.buf, 0, sens1_buffer.bytes );
	sens1_buffer.w_idx = 0;
	sens_trg_count = 0;
	status = MS_MEASUREMENT_READY;
}

/**
 * @brief 	Set the integration time in us for the sensor.
 *
 * The minimum is defined by MIN_INTERGATION_TIME ( 50 us )
 * The maximum is (UINT32_MAX / TIM2_SCALER) - PRE_ST_DELAY ( ~4200 sec )
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
 * @ brief Start a single measurement.
 *
 *	All further work is done in by the timers TIM1 and TIM2 and in the GPIO ISR.
 *	1. TIM2 generating the start signal (ST) for the sensor
 *	2. TIM1 count the trigger pulses (TRG) from the sensor directly after
 * todo	ST goes low and throw an IR when MSPARAM_TRG_CNT many pulses occurred.
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

	__HAL_TIM_SET_AUTORELOAD( &htim2, int_time_cnt );

	// enable TIM channels
	// Don't use TIM_CCxChannelCmd() because it will generate a short
	// uncertain state, which will result in a high with an external
	// pull-up resistor.

	// enable TIM1 IRs: update and channel 4
	__HAL_TIM_CLEAR_IT( &htim1, TIM_IT_UPDATE );
	__HAL_TIM_ENABLE_IT( &htim1, TIM_IT_UPDATE );
	__HAL_TIM_CLEAR_IT( &htim1, TIM_IT_CC4 );
	__HAL_TIM_ENABLE_IT( &htim1, TIM_IT_CC4 );

	// prepare tim1 channel 2 for capturing EOS
	TIM1->CCER |= TIM_CCER_CC2E;
	TIM1->CCR2 = 0;

	// enable tim1 channel 4 to output TEST
	TIM1->CCER |= TIM_CCER_CC4E;
	__HAL_TIM_MOE_ENABLE( &htim1 );

	// enable tim2 channel 3 to output ST and start tim2
	TIM2->CCER |= TIM_CCER_CC3E;
	TIM2->CR1 |= TIM_CR1_CEN;
	status = MS_MEASUREMENT_STARTED;
}

void micro_spec_wait_for_measurement_done( void )
{
	while( status != MS_MEASUREMENT_ONGOING_TIM1_UP )
	{
		// busy waiting
	}

	post_process_values();
	status = MS_MEASUREMENT_DONE;
}

/**
 * micro_spec_post_process_values()
 *
 *
 * Insitu reorder values, as the single bits are not in the correct order.
 *
 */
static void post_process_values( void )
{
	/*
	 * before ordering:
	 *   PC[7..0] PA[7..0]
	 *   c7 c6 c5 c4 c3 c2 c1 c0 a7 a6 a5 a4 a3 a2 a1 a0
	 *
	 * after ordering:
	 *   c3 c2 a0 a1 a4 c1 c0 a3 a2 c7 a7 a6 a5 c6 c5 c4
	 *
	 */
	uint16_t res, val;
	int16_t i;

	if( DBG_SIMULATE_SENSOR )
	{
		return;
	}

	for( i = 0; i < sens1_buffer.w_idx; ++i )
	{
		val = sens1_buffer.buf[i];

		res = 0;
		res |= (val >> 11) & BIT0; //PC3
		res |= (val >> 9) & BIT1;  //PC2
		res |= (val << 2) & BIT2;  //PA0
		res |= (val << 2) & BIT3;  //PA1
		res |= (val << 0) & BIT4;  //PA4
		res |= (val >> 4) & BIT5;  //PC1
		res |= (val >> 2) & BIT6;  //PC0
		res |= (val << 4) & BIT7;  //PA3
		res |= (val << 6) & BIT8;  //PA2
		res |= (val >> 6) & BIT9;  //PC7
		res |= (val << 3) & BIT10; //PA7
		res |= (val << 5) & BIT11; //PA6
		res |= (val << 7) & BIT12; //PA5
		res |= (val >> 1) & BIT13; //PC6
		res |= (val << 1) & BIT14; //PC5
		res |= (val << 3) & BIT15; //PC4

		sens1_buffer.buf[i] = res;
	}

	/* We captured the TIM1 counter value at the moment when EOS occurred
	 * on the TIM1 channel2. This indicated the last valid data the sensor
	 * was sending. Also the ADC already processed the last data, as EOS
	 * is set high between two edges of the Sensor TRG signal.*/
	sens1_buffer.last_valid = TIM1->CCR2;
}

/**
 * Enables the clock signal for the sensor.
 * This gated through the sensor (with a small delay ~43ns)
 * and given back as TRG-signal from the sensor to the ADC-
 * trigger input.
 */
void enable_sensor_clk( void )
{
	HAL_GPIO_WritePin( SENS_CLK_GPIO_Port, SENS_CLK_Pin, GPIO_PIN_RESET );
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
	HAL_GPIO_WritePin( SENS_CLK_GPIO_Port, SENS_CLK_Pin, GPIO_PIN_RESET );
	GPIO_InitTypeDef GPIO_InitStruct;
	/*Configure GPIO pin for the Sensors CLK
	 * STM32 --> SENS1 & SENS2*/
	GPIO_InitStruct.Pin = SENS_CLK_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init( SENS_CLK_GPIO_Port, &GPIO_InitStruct );

}

