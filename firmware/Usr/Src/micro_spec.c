/*
 * micro_spec.c
 *
 *  Created on: May 16, 2017
 *      Author: Bert Palm
 *
 *	General Procedure:
 *	-------------------
 *
 *	0. TIM3 provide the CLK
 *
 *	1. TIM2 is started and generate the start signal (ST) for the sensor with the
 *	length of integration time - 48us.
 *
 *	2. With the falling edge of ST (TIM2 counter reached TIM2_ARR) TIM1 is started
 *	automatically and count the rising edges of the sensors trigger (TRG) signal.
 *	(Actually we count the falling edges of the inverted TRG signal, what is the
 *	same, but the inversion is needed for the ADC).
 *
 *	3. When TIM1 counter reaches the value in its CCR4 the IR for the ADC_BUSY is
 *	enabled. With every rising edge on the ADC_BUSY line a new conversion is done
 *	and we read the result on the ADC parallel port.
 *
 *	4. Somewhat later, with the 89th TRG edge after ST was falling, the first
 *	**valid** value is send from the sensor to the ADC and slightly later we will
 *	read it from the parallel port of the ADC. This procedure repeats for 288 times
 *	until all valid values are transmitted.
 *
 *	5. After 288 valid values (should be on the 377(288+88+1) TRG edge) the sensor
 *	generates the end of scan (EOS) signal. We capture the current TRG count in
 *	the TIM1 CCR2 and an IR is generated. There we disable the IR for the
 *	ADC_BUSY line and return to the main program which should be waiting
 *	in the function sens_wait_for_measurement_done(). At this point we know
 *	that the last written value in the data buffer is valid and the first valid lies
 *	288 values before the last one.
 *
 *	Safety Mechanisms / Error Handling:
 *	------------------------------------
 *
 *	a) kind of watch dog
 *	For any reason may none or not enough TRG pulses occur. This would lock
 *	the whole program as we wait for the MS_MEASUREMENT_DONE flag in the function
 *	micro_spec_wait_for_measurement_done(). Therefore we set a fourth timer (TIM5)
 *	clocked by the internal clock and wait for 500us. If the timer is not disabled
 *	before it will throw an IR where we unlock the program and return to a certain
 *	state and of course we will set an error-flag.
 *
 *
 *	Call hierarchy for this module
 *	-------------------------------
 *
 *      1. sens_init()					    \n
 *      2. sens_measure()                                   \n
 *     (3.) when a new measurement is desired go back to 2. \n
 *      4. sens_deinit()                                    \n
 */

#include "micro_spec.h"
#include "global_config.h"
#include "tim_usr.h"
#include <string.h>

//static uint16_t mem_block1[SENSOR_DATA_BUFFER_MAX_WORDS + 1];
static canary_memblock_t secure_memblock1;

static sensor_buffer_t sens_buf = { SENSOR_DATA_BUFFER_SIZE,
SENSOR_DATA_BUFFER_MAX_WORDS, secure_memblock1.memblock, secure_memblock1.memblock };

/* Handle for the micro sprectrometer */
sensor_t sens1 = { SENS_UNINITIALIZED, &sens_buf, DEFAULT_INTEGRATION_TIME, ERRC_UNKNOWN};

static void post_process_values(void);
static void wait_for_measure_done(void);
static uint32_t map_status2errcode(uint8_t status);

/**
 * Init all internal data structs and buffer
 * and timer needed by the sensor.
 */
void sensor_init(void) {
	sens1.data = &sens_buf;
	sens1.data->base = secure_memblock1.memblock;
	sens1.data->wptr = secure_memblock1.memblock;
	sens1.errc = ERRC_UNKNOWN;

	// enable TIM channels

	// Do not use TIM_CCxChannelCmd() see
	// tim2_Init() in usr_tim.c for explanation
	// use TIMx->CCER |= TIM_CCER_CCyE

	// TIM2 for: output ST
	// en channel
	TIM2->CCER |= TIM_CCER_CC3E;

	// TIM1 for: count TRG(clock-source), capture EOS,
	// [output TEST enable EXTI2(capture Data) in its ISR]
	//
	// (EOS) en channel
	TIM1->CCER |= TIM_CCER_CC2E;

	// (TEST) en channel, en IR
	TIM1->CCER |= TIM_CCER_CC4E;

	// MAIN OUT PUT enable
	__HAL_TIM_MOE_ENABLE(&htim1);
	// all CCRx-IR in the NVIC
	NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
	NVIC_EnableIRQ(TIM1_CC_IRQn);

	// TIM5 - safty timer
	// en IR in module
	__HAL_TIM_CLEAR_IT(&htim5, TIM_IT_UPDATE);
	__HAL_TIM_ENABLE_IT(&htim5, TIM_IT_UPDATE);

	// TIM3 - CLK for sensor
	TIM3->CCER |= TIM_CCER_CC3E;
	TIM3->CCER |= TIM_CCER_CC4E;

	// start CLK
	__HAL_TIM_ENABLE(&htim3);
	HAL_Delay(1);

	sens1.status = SENS_INITIALIZED;
}

void sensor_deinit(void) {
	// disable CLK
	HAL_Delay(1);
	TIM3->CR1 &= ~TIM_CR1_CEN;
	NVIC_DisableIRQ(TIM1_CC_IRQn);
	NVIC_DisableIRQ(EXTI2_IRQn);
	sens1.status = SENS_UNINITIALIZED;
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
void sensor_measure(void) {
	uint32_t int_time_cnt;

	if (sens1.status < SENS_INITIALIZED) {
		sens1.errc = ERRC_NOT_INITIALIZED;
		return;
	}
	// reset data buffer
	memset(sens1.data->base, 0, sens1.data->size);
	sens1.data->wptr = sens1.data->base;

	// set the canaries
	memset(secure_memblock1.precanary, 0xffff, CANARYSIZE * sizeof(uint16_t));
	memset(secure_memblock1.postcanary, 0xffff, CANARYSIZE * sizeof(uint16_t));

	// prevent SysTick to stretch time critical sections
	HAL_SuspendTick();

	// 48 clock-cycles are added by the sensor to "high" of the ST-signal
	// resulting in the integration-time (see c12880ma_kacc1226e.pdf)
	int_time_cnt = MAX(sens1.itime, MIN_INTERGATION_TIME);
	int_time_cnt -= ITIME_CORRECTION;

	// prepare ST
	__HAL_TIM_SET_AUTORELOAD(&htim2, int_time_cnt);

	// prepare CAPTURE DATA START
	__HAL_TIM_CLEAR_IT(&htim1, TIM_IT_CC4);
	__HAL_TIM_ENABLE_IT(&htim1, TIM_IT_CC4);

	// prepare EOS - CAPTURE DATA END
	// Reset the CCR, enable its IR
	__HAL_TIM_SET_COMPARE(&htim1, TIM_IT_CC2, 0);
	__HAL_TIM_CLEAR_IT(&htim1, TIM_IT_CC2);
	__HAL_TIM_ENABLE_IT(&htim1, TIM_IT_CC2);

	// lets go
	__HAL_TIM_ENABLE(&htim2);
	sens1.status = SENS_MEASURE_STARTED;
	wait_for_measure_done();
	post_process_values();

	if (sens1.status == SENS_EOS_CAPTURED) {
		sens1.status = SENS_MEASURE_DONE;
	}
	sens1.errc = map_status2errcode(sens1.status);
}

/**
 * Call this if a measurement was started and we wait until
 * it is finished.
 *
 * If a new measurement after this is desired call micro_spec_measure_init().
 *
 */
static void wait_for_measure_done(void) {
	while (sens1.status < SENS_EOS_CAPTURED) {
		// busy waiting
	}
	__HAL_TIM_DISABLE_IT(&htim5, TIM_IT_UPDATE);
	HAL_ResumeTick();
	// this ensures TIM5 is done
	HAL_Delay(1);
}

/**
 *
 *
 * Local helper for insitu reorder values, as the single bits
 * are not in the correct order.
 *
 * before ordering:
 *             PC[7..0]           __
 * PORT          ... c7 c6 c5 c4 |c3| c2 c1 c0
 * BIT           ... 07 06 05 04 |03| 02 01 00
 * example c0:  1. x = value >> 03
 *
 *   PA[7..0]
 * PORT ... a10 -- a8 a7 a6 a5 a4 a3 a2 a1 a0
 * BIT  ...  10 09 08 07 06 05 04 03 02 01 00
 *
 * after ordering:             __
 * d         = c7 c6 c5 a5 c0 |c3| c1 c2 a6 a7 c4 a4 a2 a8 a1 a0
 * BIT(D0-D15) 15 14 13 12 11 |10| 09 08 07 06 05 04 03 02 01 00
 * example c0:  2. d |= x << 10
 */
/**
 *
 *
 * Local helper for insitu reorder values, as the single bits
 * are not in the correct order.
 *
 * before ordering:
 *             	PC[15..0]                  __      PA[15..0]
 * value   ... -- -- c7 c6 c5 c4 c3 c2 c1 |c0| -- -- -- -- -- a10 -- a8 a7 a6 a5 a4 a3 a2 a1 a0
 * BIT     ... 25 24 23 22 21 20 19 18 17 |16| 15 14 13 12 11  10 09 08 07 06 05 04 03 02 01 00
 * example c0:  1. x= value >> 16
 *
 * after ordering:                __
 * d           -- -- c7 c6 c5 a5 |c0| c3 c1 c2 a6 a7 c4 a4 a10 a8 a1 a0
 * BIT(D0-D15) 17 16 15 14 13 12 |11| 10 09 08 07 06 05 04 03  02 01 00
 * example c0:  2. d |= x << 11
 */
static void post_process_values(void) {
	uint8_t x;
	uint32_t d, val;
	uint32_t *rptr = sens1.data->base;

	rptr = sens1.data->base;
	while (rptr < sens1.data->wptr) {
		val = *rptr;
		d = 0;

		// a0
		x = (val >> 0) & 0x01;
		d |= (x << 0);

		// a1
		x = (val >> 1) & 0x01;
		d |= (x << 1);

		// a2
		x = (val >> 2) & 0x01;
		d |= (x << 3);

		// a3
		x = (val >> 3) & 0x01;
		d |= (x << 2);

		// a4
		x = (val >> 4) & 0x01;
		d |= (x << 4);

		// a5
		x = (val >> 5) & 0x01;
		d |= (x << 12);

		// a6
		x = (val >> 6) & 0x01;
		d |= (x << 7);

		// a7
		x = (val >> 7) & 0x01;
		d |= (x << 6);

		// C0
		x = (val >> 8) & 0x01;
		d |= (x << 11);

		// C1
		x = (val >> 9) & 0x01;
		d |= (x << 9);

		// C2
		x = (val >> 10) & 0x01;
		d |= (x << 8);

		// C3
		x = (val >> 11) & 0x01;
		d |= (x << 10);

		// C4
		x = (val >> 12) & 0x01;
		d |= (x << 5);

		// C5
		x = (val >> 13) & 0x01;
		d |= (x << 13);

		// C6
		x = (val >> 14) & 0x01;
		d |= (x << 14);

		// C7
		x = (val >> 15) & 0x01;
		d |= (x << 15);

		*rptr = d;
		rptr++;
	}
}

/**
 * @brief 	Set the integration time in us for the sensor.
 *
 * The minimum is defined by MIN_INTERGATION_TIME
 * The maximum by MAX_INTERGATION_TIME
 *
 * @param int_time	The integration time in us
 * @return The integration time value set
 */
uint32_t sensor_set_itime(uint32_t itime) {

	if (itime < MIN_INTERGATION_TIME) {
		sens1.itime = MIN_INTERGATION_TIME;
	} else if (itime > MAX_INTERGATION_TIME) {
		sens1.itime = MAX_INTERGATION_TIME;
	} else {
		sens1.itime = itime;
	}
	return sens1.itime;
}


/**
 * Map the sensor status to the appropriate error code.
 */
static uint32_t map_status2errcode(uint8_t status) {
	switch (status) {
	case SENS_MEASURE_DONE:
		return ERRC_NO_ERROR;
	case SENS_ERR_TIMEOUT:
		return ERRC_TIMEOUT;
	case SENS_ERR_NO_EOS:
		return ERRC_NO_EOS;
	case SENS_ERR_EOS_EARLY:
		return ERRC_EOS_EARLY;
	default:
		return ERRC_UNKNOWN;
	}
}
