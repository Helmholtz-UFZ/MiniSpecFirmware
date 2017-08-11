/*
 * usr_main.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include "main.h"
#include "global_include.h"
#include "micro_spec.h"
#include "usart.h"
#include "tim.h"

static void send_data( uint8_t format );
static void usr_main_error_handler( uint8_t err );
static uint8_t data_format = DATA_FORMAT_ASCII; //hack
static bool stream_mode = 0;

int usr_main( void )
{
	uint8_t err = 0;

	// we enable IRs where/when we need them
	HAL_NVIC_DisableIRQ( USART3_IRQn );
	HAL_NVIC_DisableIRQ( TIM1_CC_IRQn );
	HAL_NVIC_DisableIRQ( TIM1_UP_TIM16_IRQn );
	HAL_NVIC_DisableIRQ( EXTI2_IRQn );

	usart3_init();
	tim1_Init();
	tim2_Init();

	/* Run the system ------------------------------------------------------------*/

	// enabling usart receiving
	NVIC_EnableIRQ( USART3_IRQn );
	HAL_UART_Receive_IT( &huart3, uart3_rx_buffer.base, uart3_rx_buffer.size );

	uart_printf( &huart3, &uart3_tx_buffer, "\nstart\n" );
	while( 1 )
	{
		// check if we received a usr command
		usart3_receive_handler();

		switch( usrcmd ) {
		case USR_CMD_SINGLE_MEASURE_START:
			micro_spec_init();
			err = micro_spec_measure_init();
			if( err ) break;
			err = micro_spec_measure_start();
			if( err ) break;
			err = micro_spec_wait_for_measurement_done();
			if( err ) break;
			micro_spec_deinit();
			send_data( data_format );
			break;

		case USR_CMD_WRITE_INTEGRATION_TIME:
			micro_spec_set_integration_time( usr_cmd_data );
			break;

		case USR_CMD_READ_INTEGRATION_TIME:
			if( data_format == DATA_FORMAT_BIN )
			{
				HAL_UART_Transmit( &huart3, (uint8_t *) &hms1.integrtion_time, 4, 1000 );
			}
			else
			{
				uart_printf( &huart3, &uart3_tx_buffer, "integration time = %ld us\n", hms1.integrtion_time );
			}
			break;

		case USR_CMD_CONTINUOUS_MEASURE_START:
			micro_spec_init();
			stream_mode = 1;
			break;

		case USR_CMD_SET_DATA_FORMAT:
			data_format = (usr_cmd_data > 0) ? DATA_FORMAT_ASCII : DATA_FORMAT_BIN;
			break;

		case USR_CMD_CONTINUOUS_MEASURE_END:
			micro_spec_deinit();
			stream_mode = 0;
			break;

		default:
			break;
		}

		if( stream_mode )
		{
			err = micro_spec_measure_init();
			if( err ) break;
			err = micro_spec_measure_start();
			if( err ) break;
			err = micro_spec_wait_for_measurement_done();
			if( err ) break;
			send_data( data_format );
			/*
			 * todo IF stream en -> save command (save the last data)
			 * -> get command (send data via uart) delay next measurement a bit
			 * -> check rx_buffer after measurement (only in stream mode) for commands
			 *
			 *
			 */
		}

		if( err )
		{
			usr_main_error_handler( hms1.status );
			err = 0;
		}
	}
	
	return 0;
}

/**
 * Local helper for sending data via the uart3 interface.
 *
 * \param format	0 (DATA_FORMAT_BIN) send raw data, byte per byte.(eg. 1000 -> 0xE8 0x03)\n
 * \param format	1 (DATA_FORMAT_ASCII) send the data as in ASCII, as human readable text. (eg. 1000 -> '1' '0' '0' '0')
 *
 *
 */
static void send_data( uint8_t format )
{
	if( format == DATA_FORMAT_BIN )
	{
		//send data
		HAL_UART_Transmit( &huart3, (uint8_t *) (hms1.data->wptr - MSPARAM_PIXEL), MSPARAM_PIXEL, MSPARAM_PIXEL * 100 );
	}
	else
	{
		uint16_t *rptr = hms1.data->base;
		uint16_t i = 0;

		while( rptr < hms1.data->wptr )
		{
			if( DBG_SEND_ALL == OFF && rptr < (hms1.data->wptr - MSPARAM_PIXEL) )
			{
				rptr++;
				continue;
			}
			
			if( rptr == (hms1.data->wptr - MSPARAM_PIXEL) )
			{
				uart_printf( &huart3, &uart3_tx_buffer, "\n"DELIMITER_STR );
				uart_printf( &huart3, &uart3_tx_buffer, "\n"HEADER_STR );
				i = 0;
			}

			if( i % 10 == 0 )
			{
				uart_printf( &huart3, &uart3_tx_buffer, "\n%03d   %05d ", i, *rptr );
			}
			else
			{
				uart_printf( &huart3, &uart3_tx_buffer, "%05d ", *rptr );
			}

			rptr++;
			i++;
		}
		uart_printf( &huart3, &uart3_tx_buffer, "\n"DELIMITER_STR"\n\n" );
	}
}

static void usr_main_error_handler( uint8_t err )
{
	switch( err ) {
	case 0:
		return;

	case MS_MEASUREMENT_ERR_TIMEOUT:
		if( data_format == DATA_FORMAT_ASCII )
		{
			uart_printf( &huart3, &uart3_tx_buffer, "ERR: TIMEOUT. Please check the following:\n"
				"1. is sensor plugged ?\n"
				"2. ADC/sensor powered ?\n"
				"3. check physical connections" );
		}
		stream_mode = 0;
		micro_spec_deinit();
		break;

	case MS_MEASUREMENT_ERR_NO_EOS:
		if( data_format == DATA_FORMAT_ASCII )
		{
			uart_printf( &huart3, &uart3_tx_buffer, "ERR: NO EOS. Something went wrong, please debug manually.\n" );
		}
		stream_mode = 0;
		micro_spec_deinit();
		break;

	default:
		break;
	}
}
