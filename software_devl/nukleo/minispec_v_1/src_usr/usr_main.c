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

int usr_main( void )
{

	usart3_init();
	tim1_Init();
	tim2_Init();
	micro_spec_init();

	/* Initialize interrupts */

	// Enable TIM capture compare IRs
	NVIC_EnableIRQ( TIM1_UP_TIM16_IRQn );
	NVIC_EnableIRQ( TIM1_CC_IRQn );

	// EXTI2_IRQn_BUSY1:	 en/dis in TIM1 ISR
	//	HAL_NVIC_SetPriority(EXTI2_IRQn_BUSY1,1,0);

	NVIC_EnableIRQ( USART3_IRQn );
	//	HAL_NVIC_SetPriority(EXTI2_IRQn_BUSY1,2,0);

	/* Run the system ------------------------------------------------------------*/

	// enabling usart receiving
	HAL_UART_Receive_IT( &huart3, uart3_recv_buffer.base, uart3_recv_buffer.size );

	bool continiuos_mode = 0;
	while( 1 )
	{
		// check if we received a usr command
		usart3_receive_handler();

		switch( usrcmd ) {
		case USR_CMD_SINGLE_MEASURE_START:
			micro_spec_measure_init();
			micro_spec_measure_start();
			micro_spec_wait_for_measurement_done();
			micro_spec_measure_deinit();

			uint16_t cap_st = MSPARAM_CAPTURE_PXL_ST;
			//send data
			HAL_UART_Transmit( &huart3, (uint8_t *) &cap_st, 2, 100 );
			HAL_UART_Transmit( &huart3, (uint8_t *) &sens1_buffer.w_idx, 2, 100 );
			HAL_UART_Transmit( &huart3, (uint8_t *) &sens1_buffer.last_valid, 2, 100 );
			HAL_UART_Transmit( &huart3, (uint8_t *) sens1_buffer.buf, sens1_buffer.bytes, 100 );
			break;

		case USR_CMD_WRITE_INTEGRATION_TIME:
			micro_spec_set_integration_time( usr_cmd_data );
			break;

		case USR_CMD_READ_INTEGRATION_TIME:
			HAL_UART_Transmit( &huart3, (uint8_t *) &integrtion_time, 4, 1000 );
			break;

		case USR_CMD_CONTINUOUS_MEASURE_START:
			micro_spec_measure_init();
			continiuos_mode = 1;
			break;

		case USR_CMD_CONTINUOUS_MEASURE_END:
			micro_spec_measure_deinit();
			continiuos_mode = 0;
			break;

		default:
			break;
		}

		if( continiuos_mode )
		{
			micro_spec_measure_start();
			micro_spec_wait_for_measurement_done();
			micro_spec_post_process_values();

			//send data
			HAL_UART_Transmit( &huart3, (uint8_t *) sens1_buffer.buf, sens1_buffer.bytes, 100 );
			//send delimiter
			HAL_UART_Transmit( &huart3, (uint8_t *) delim, sizeof(delim), 100 );
		}

		HAL_Delay( 5 );
	}

	return 0;
}
