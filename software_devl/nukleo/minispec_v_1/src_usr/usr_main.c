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

static const uint8_t delim[16] =
        { 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF };

static uint8_t format = DATA_FORMAT_ASCII; //hack

static void send_data( void );

int usr_main( void )
{
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

	bool continiuos_mode = 0;
	while( 1 )
	{
		// check if we received a usr command
		usart3_receive_handler();

		switch( usrcmd ) {
		case USR_CMD_SINGLE_MEASURE_START:
			micro_spec_init();
			micro_spec_measure_init();
			micro_spec_measure_start();
			micro_spec_wait_for_measurement_done();
			micro_spec_deinit();
			send_data();
			break;

		case USR_CMD_WRITE_INTEGRATION_TIME:
			micro_spec_set_integration_time( usr_cmd_data );
			break;

		case USR_CMD_READ_INTEGRATION_TIME:
			if( format == DATA_FORMAT_BIN )
			{
				HAL_UART_Transmit( &huart3, (uint8_t *) &integrtion_time, 4, 1000 );
			}
			else
			{
				uart_printf( &huart3, &uart3_tx_buffer, "integration time = %ld us\n", integrtion_time );
			}
			break;

		case USR_CMD_CONTINUOUS_MEASURE_START:
			micro_spec_init();
			continiuos_mode = 1;
			break;

		case USR_CMD_SET_DATA_FORMAT:
			format = (usr_cmd_data > 0) ? DATA_FORMAT_ASCII : DATA_FORMAT_BIN;
			break;

		case USR_CMD_CONTINUOUS_MEASURE_END:
			micro_spec_deinit();
			continiuos_mode = 0;
			break;

		default:
			break;
		}

		if( continiuos_mode )
		{
			micro_spec_measure_init();
			micro_spec_measure_start();
			micro_spec_wait_for_measurement_done();
			send_data();

		}
	}
	
	return 0;
}

static void send_data( void )
{
	if( format == DATA_FORMAT_BIN )
	{
		// send metadata
//		HAL_UART_Transmit( &huart3, (uint8_t *) &sens1_buffer.w_idx, 2, 100 );
//		HAL_UART_Transmit( &huart3, (uint8_t *) &sens1_buffer.last_valid, 2, 100 );

//send data
		HAL_UART_Transmit( &huart3, (uint8_t *) sens1_buffer.buf, sens1_buffer.bytes, 100 );

		//send delimiter
		HAL_UART_Transmit( &huart3, (uint8_t *) delim, sizeof(delim), 100 );
	}
	else
	{
		uint16_t i;
		for( i = 0; i < sens1_buffer.w_idx; ++i )
		{

			if( i % 10 == 0 )
			{
				uart_printf( &huart3, &uart3_tx_buffer, "\n%05d ", sens1_buffer.buf[i] );
			}
			else
			{
				uart_printf( &huart3, &uart3_tx_buffer, "%05d ", sens1_buffer.buf[i] );
			}
		}
		uart_printf( &huart3, &uart3_tx_buffer, "\n\n" );
	}
}
