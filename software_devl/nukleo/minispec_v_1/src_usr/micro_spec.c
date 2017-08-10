/*
 * micro_spec.c
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 *
 *	General Procedure:
 *	-------------------
 *
 *	1. TIM2 is started and generate the start signal (ST) for the sensor with the
 *	length of integration time - 48us.
 *
 *	2. With the falling edge of ST (TIM2 counter reached TIM2ARR) TIM1 is started
 *	automatically and count the rising edges of the sensors trigger (TRG) signal.
 *	(Actually we count the falling edges of the inverted TRG signal, what is the
 *	same, but the inversion is needed for the ADC).
 *
 *	3. When TIM1 counter reaches the value in the CCR4 the IR for the ADC_BUSY is
 *	enabled. With every rising edge on the ADC_BUSY line a new conversion is done
 *	and we read the result on the ADC parallel port.
 *
 *	4. Somewhat later, with the 89th TRG edge after ST was falling, the first
 *	**valid** value is send from the sensor to the ADC and slightly later we will
 *	read it from the parallel port of the ADC. This procedure repeats for 288 times
 *	until all valid values are transmitted.
 *
 *	5. After 288 valid values (should be on the 377(288+88+1) TRG edge) the sensor
 *	generates the end of signal (EOS) signal. We capture the current TRG count in
 *	the TIM1 CCR2 and an IR is generated. todo There we disable the IR for the
 *	ADC_BUSY line, stop TIM1 and return to the main program which should be waiting
 *	in the function micro_spec_wait_for_measurement_done(). At this point we know
 *	that the last written value in the data buffer is valid and the first valid lies
 *	288 values before the last one.
 *
 *	Safety Mechanisms / Error Handling:
 *	------------------------------------
 *
 *	a) todo If and only if EOS is not generated or not detected by any mischance.
 *	The TIM1 counter will count up to TIM1ARR, where we throw an IR and do the same
 *	as procedure as described in 5. additional we set a warning-flag to inform about
 *	the missing EOS.
 *
 *	b) todo For any reason may none or not enough TRG pulses occur. This would lock
 *	the whole program as we wait for the MS_MEASUREMENT_DONE flag in the function
 *	micro_spec_wait_for_measurement_done(). Therefore we set a third timer clocked
 *	by the internal clock and wait for the time 400 TRG pulses would normally need.
 *	If the timer is not disabled before it will throw an IR where we unlock the
 *	program and return to a certain state and of course we will set an error-flag.
 */

#include "stm32l4xx_hal.h"
#include <string.h>
#include "micro_spec.h"
#include "global_include.h"
#include "tim.h"

/* micro sprectrometer 1 handle */
microspec_t hms1 =
        { MS_UNINITIALIZED, NULL, MSPARAM_DEFAULT_INTTIME };

static uint16_t mem_block1[MICROSPEC_DATA_BUFFER_MAX_WORDS + 1];
static microspec_buffer ms1_buf =
        { MICROSPEC_DATA_BUFFER_SIZE, MICROSPEC_DATA_BUFFER_MAX_WORDS, mem_block1, mem_block1, 0 };

static void enable_sensor_clk( void );
static void disable_sensor_clk( void );
static void post_process_values( void );

/**
 *@brief Init all internal data structs and buffer for the sensor.
 */
void micro_spec_init( void )
{
	hms1.data = &ms1_buf;
	hms1.data->base = mem_block1;
	hms1.data->last_valid = 0;
	hms1.data->wptr = mem_block1;

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

	// Enable TIM1 IRs
	NVIC_ClearPendingIRQ( TIM1_UP_TIM16_IRQn );
	NVIC_ClearPendingIRQ( TIM1_CC_IRQn );
	NVIC_EnableIRQ( TIM1_UP_TIM16_IRQn );
	NVIC_EnableIRQ( TIM1_CC_IRQn );

	enable_sensor_clk();
	HAL_Delay( 1 );
	hms1.status = MS_INITIALIZED;
}

void micro_spec_deinit( void )
{
	HAL_Delay( 1 );
	disable_sensor_clk();

	// Enable TIM1 IRs
	NVIC_DisableIRQ( TIM1_UP_TIM16_IRQn );
	NVIC_DisableIRQ( TIM1_CC_IRQn );
	hms1.status = MS_UNINITIALIZED;
}

/**
 * Init the spectrometer for a single measurement.
 */
void micro_spec_measure_init( void )
{
	if( hms1.status != MS_INITIALIZED )
	{
		return;
	}
	memset( hms1.data->base, 0, hms1.data->size2 );
	hms1.data->wptr = hms1.data->base;
	hms1.status = MS_MEASUREMENT_READY;
}

/**
 * @ brief Start a single measurement.
 *
 *	All further work is done in by the timers TIM1 and TIM2 and in the GPIO ISR.
 *	The procedure is described in the header.
 *
 */
void micro_spec_measure_start( void )
{

	if( !(hms1.status == MS_MEASUREMENT_READY || hms1.status == MS_MEASUREMENT_DONE) )
	{
		return;
	}
	uint32_t int_time_cnt;

	// 48 clock-cycles are added to ST-signal-"high" resulting in integrationtime
	// (see c12880ma_kacc1226e.pdf)
	const uint8_t clk_cycl = 48;

	int_time_cnt = MAX( hms1.integrtion_time, MIN_INTERGATION_TIME );
	int_time_cnt -= clk_cycl;

	__HAL_TIM_SET_AUTORELOAD( &htim2, int_time_cnt );

	// reset the capturing reg for the eos
	TIM1->CCR2 = 0;

	TIM2->CR1 |= TIM_CR1_CEN;
	hms1.status = MS_MEASUREMENT_STARTED;
}

void micro_spec_wait_for_measurement_done( void )
{
	if( hms1.status != MS_MEASUREMENT_STARTED )
	{
		return;
	}

	while( hms1.status != MS_MEASUREMENT_ONGOING_TIM1_UP )
	{
		// busy waiting
	}

	post_process_values();
	hms1.status = MS_MEASUREMENT_DONE;
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
	uint16_t *rptr = hms1.data->base;

	if( DBG_SIMULATE_SENSOR )
	{
		return;
	}

	while( rptr < hms1.data->wptr )
	{
		val = *rptr;

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

		*rptr = res;
		rptr++;
	}

	/* We captured the TIM1 counter value at the moment when EOS occurred
	 * on the TIM1 channel2. This indicated the last valid data the sensor
	 * was sending. Also the ADC already processed the last data, as EOS
	 * is set high between two edges of the Sensor TRG signal.*/
	hms1.data->last_valid = TIM1->CCR2;
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
		hms1.integrtion_time = MIN_INTERGATION_TIME;
	}
	else
	{
		hms1.integrtion_time = int_time;
	}

	return hms1.integrtion_time;
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

