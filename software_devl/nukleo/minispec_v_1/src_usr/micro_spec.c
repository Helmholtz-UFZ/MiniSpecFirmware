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
 *	the TIM1 CCR2 and an IR is generated. There we disable the IR for the
 *	ADC_BUSY line, stop TIM1 and return to the main program which should be waiting
 *	in the function micro_spec_wait_for_measurement_done(). At this point we know
 *	that the last written value in the data buffer is valid and the first valid lies
 *	288 values before the last one.
 *
 *	Safety Mechanisms / Error Handling:
 *	------------------------------------
 *
 *	a) If and only if EOS is not generated or not detected by any mischance.
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
 *
 *
 *	Call hierarchy for this module
 *	-------------------------------
 *
 *	1. micro_spec_init()					\n
 * 	2. micro_spec_measure_init()                            \n
 * 	3. micro_spec_measure_start()                           \n
 * 	4. micro_spec_wait_for_measurement_done()               \n
 * 	5. when a new measurement is desired go back to 2.      \n
 * 	6. micro_spec_deinit()                                  \n
 */

#include "stm32l4xx_hal.h"
#include <string.h>
#include "micro_spec.h"
#include "global_include.h"
#include "tim.h"

static uint16_t mem_block1[MICROSPEC_DATA_BUFFER_MAX_WORDS + 1];

static microspec_buffer_t ms1_buf =
        { MICROSPEC_DATA_BUFFER_SIZE, MICROSPEC_DATA_BUFFER_MAX_WORDS, mem_block1, mem_block1 };

/* Handle for the micro sprectrometer 1 */
microspec_t hms1 =
        { MS_UNINITIALIZED, &ms1_buf, MSPARAM_DEFAULT_INTTIME };

static void enable_sensor_clk( void );
static void disable_sensor_clk( void );
static void post_process_values( void );

/**
 *Init all internal data structs and buffer needed by the sensor(s).
 */
void micro_spec_init( void )
{
	hms1.data = &ms1_buf;
	hms1.data->base = mem_block1;
	hms1.data->wptr = mem_block1;

	// enable TIM channels
	// Don't use TIM_CCxChannelCmd() (which also use the HAL) because it
	// will generate a short uncertain state, which will result in a high
	// with an external pull-up resistor (as the level-translator has internal!).
	// TIM_CCxChannelCmd() is also used by the HAL.

	//TIM1
	// enable the IR's for channel 2 and 4 in the module
	__HAL_TIM_CLEAR_IT( &htim1, TIM_IT_CC4 );
	__HAL_TIM_ENABLE_IT( &htim1, TIM_IT_CC4 );
	__HAL_TIM_CLEAR_IT( &htim1, TIM_IT_CC2 );
	__HAL_TIM_ENABLE_IT( &htim1, TIM_IT_CC2 );

	// prepare channel 2 for capturing EOS
	TIM1->CCER |= TIM_CCER_CC2E;
	TIM1->CCR2 = 0;

	// enable channel 4 to output TEST
	TIM1->CCER |= TIM_CCER_CC4E;
	__HAL_TIM_MOE_ENABLE( &htim1 );

	// Enable IR as we want to end the measuring sequence if
	// we capture EOS
	NVIC_ClearPendingIRQ( TIM1_CC_IRQn );
	NVIC_EnableIRQ( TIM1_CC_IRQn );

	//TIM2
	// enable tim2 channel 3 to output ST
	TIM2->CCER |= TIM_CCER_CC3E;

	//TIM5
	__HAL_TIM_CLEAR_IT( &htim5, TIM_IT_UPDATE );
	__HAL_TIM_ENABLE_IT( &htim5, TIM_IT_UPDATE );

	enable_sensor_clk();
	HAL_Delay( 1 );
	hms1.status = MS_INITIALIZED;
}

void micro_spec_deinit( void )
{
	HAL_Delay( 1 );
	disable_sensor_clk();
	NVIC_DisableIRQ( TIM1_CC_IRQn );
	hms1.status = MS_UNINITIALIZED;
}

/**
 * Init the spectrometer for a single measurement.
 *
 * This should be called every time a before a new measurement
 * is made.
 */
uint8_t micro_spec_measure_init( void )
{
	if( !(hms1.status == MS_INITIALIZED || hms1.status == MS_MEASUREMENT_DONE) )
	{
		return 1;
	}
	memset( hms1.data->base, 0, hms1.data->size );
	hms1.data->wptr = hms1.data->base;
	hms1.status = MS_MEASUREMENT_READY;
	return 0;
}

/**
 * @ brief Start a single measurement.
 *
 * All further work is done in by the timers TIM1 and TIM2 and in the GPIO ISR.
 * The procedure is described in the header.
 *
 * Call micro_spec_measure_init() before this.
 *
 * After this call micro_spec_wait_for_measurement_done() to wait until the
 * measurement is done.
 *
 */
uint8_t micro_spec_measure_start( void )
{

	if( hms1.status < MS_MEASUREMENT_READY )
	{
		return 1;
	}

	uint32_t int_time_cnt;

	HAL_SuspendTick();
	//todo disable uart ?? ,ove the above somewhere else??

	// 48 clock-cycles are added to ST-signal-"high" resulting in integrationtime
	// (see c12880ma_kacc1226e.pdf)
	const uint8_t clk_cycl = 48;	//todo define

	int_time_cnt = MAX( hms1.integrtion_time, MIN_INTERGATION_TIME );
	int_time_cnt -= clk_cycl;

	// EOS prepare.
	// Reset the capturing reg and enable the capturing IR
	TIM1->CCR2 = 0;
	__HAL_TIM_CLEAR_IT( &htim1, TIM_IT_CC2 );
	__HAL_TIM_ENABLE_IT( &htim1, TIM_IT_CC2 );

	// set the countervalue for integrationtime on TIM2
	__HAL_TIM_SET_AUTORELOAD( &htim2, int_time_cnt );

	TIM2->CR1 |= TIM_CR1_CEN;
	hms1.status = MS_MEASUREMENT_STARTED;
	return 0;
}

/**
 * Call this if a measurement was started and we wait until
 * it is finished.
 *
 * If a new measurement after this is desired call micro_spec_measure_init().
 *
 */
uint8_t micro_spec_wait_for_measurement_done( void )
{
	if( hms1.status < MS_MEASUREMENT_STARTED )
	{
		HAL_ResumeTick();
		return 1;
	}

	while( hms1.status < MS_MEASUREMENT_CAPTURED_EOS )
	{
		// busy waiting
	}

	__HAL_TIM_DISABLE_IT( &htim5, TIM_IT_UPDATE );

	HAL_ResumeTick();

	post_process_values();

	if( hms1.status == MS_MEASUREMENT_CAPTURED_EOS )
	{
		hms1.status = MS_MEASUREMENT_DONE;
		return 0;
	}
	else
	{
		return 1;
	}
}

/**
 *
 *
 * Local helper for insitu reorder values, as the single bits
 * are not in the correct order.
 *
 * before ordering:
 *             PC[7..0]                PA[7..0]
 * buffer[i] = c7 c6 c5 c4 c3 c2 c1 c0 a7 a6 a5 a4 a3 a2 a1 a0
 *
 * after ordering:
 * buffer[i] = c3 c2 a0 a1 a4 c1 c0 a3 a2 c7 a7 a6 a5 c6 c5 c4
 *
 *
 */
static void post_process_values( void )
{

	uint16_t res, val;
	uint16_t *rptr = hms1.data->base;

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
}

/**
 * @brief 	Set the integration time in us for the sensor.
 *
 * The minimum is defined by MIN_INTERGATION_TIME ( 50 us )
 * The maximum is 1 second
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
	else if( int_time > MAX_INTERGATION_TIME )
	{
		hms1.integrtion_time = MAX_INTERGATION_TIME;
	}
	else
	{
		hms1.integrtion_time = int_time;
	}

	return hms1.integrtion_time;
}

/**
 * Enables the clock signal for the sensor.
 *
 * This clock gated through the sensor (with a small todo delay ~43ns??)
 * and is given back as TRG-signal from the sensor. The inverted
 * version of the TRG is send to the ADC-trigger input and also
 * we count the falling edges of the inverted TRG as clock source
 * of TIM1. So the sensor clock should stay enabled for the whole
 * cycle of all measurements, until this (micro-sprectrometer-)
 * module is disabled or the nukleo is powered down.
 */
void enable_sensor_clk( void )
{
	TIM3->CCER |= TIM_CCER_CC3E;
//	TIM3->CCR3 = 40;
	TIM3->CCER |= TIM_CCER_CC4E;
//	TIM3->CCR4 = 40;
	TIM3->CR1 |= TIM_CR1_CEN;
//	HAL_GPIO_WritePin( SENS_CLK_GPIO_Port, SENS_CLK_Pin, GPIO_PIN_RESET );
//	GPIO_InitTypeDef GPIO_InitStruct;
//	/*Configure GPIO pin for the Sensors CLK
//	 * STM32 --> SENS1 & SENS2*/
//	GPIO_InitStruct.Pin = SENS_CLK_Pin;
//	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//	GPIO_InitStruct.Pull = GPIO_NOPULL;
//	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//	GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
//	HAL_GPIO_Init( SENS_CLK_GPIO_Port, &GPIO_InitStruct );

}
/**
 * Disable the sensor clock. When the CLK-signal is deactivated,
 * also the TRG-signal is deactivated. @sa enable_sensor_clk()
 */
void disable_sensor_clk( void )
{
//	GPIO_InitTypeDef GPIO_InitStruct;
//	/*Configure GPIO pin for the Sensors CLK
//	 * STM32 --> SENS1 & SENS2*/
//	GPIO_InitStruct.Pin = SENS_CLK_Pin;
//	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//	GPIO_InitStruct.Pull = GPIO_NOPULL;
//	HAL_GPIO_Init( SENS_CLK_GPIO_Port, &GPIO_InitStruct );
//	HAL_GPIO_WritePin( SENS_CLK_GPIO_Port, SENS_CLK_Pin, GPIO_PIN_RESET );
}

