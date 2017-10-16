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
#include "string.h"

static void send_data( uint8_t format );
static void usr_main_error_handler( uint8_t err );

static void parse_external_command( uint8_t *buffer, uint16_t size );
static usr_cmd_enum_t usrcmd = USR_CMD_UNKNOWN;
static uint32_t usr_cmd_data = 0;

static uint8_t data_format = DATA_FORMAT_BIN;
static bool stream_mode = 0;

#define UART_USR_CHAR_MATCH_IR_ENABLE		(huart_usr.Instance->CR1 |= USART_CR1_CMIE)
#define UART_USR_CHAR_MATCH_IR_DISABLE		(huart_usr.Instance->CR1 &= ~USART_CR1_CMIE)

int usr_main( void )
{
	uint8_t err = 0;

	// we enable IRs where/when we need them
	HAL_NVIC_DisableIRQ( USART3_IRQn );
	HAL_NVIC_DisableIRQ( TIM1_CC_IRQn );
	HAL_NVIC_DisableIRQ( EXTI2_IRQn );

	usart3_init();
	tim1_Init();
	tim2_Init();
	tim5_Init();

	/* Run the system ------------------------------------------------------------*/

	// enabling usart receiving
	NVIC_EnableIRQ( USART3_IRQn );
	HAL_UART_Receive_DMA( &huart3, uart3_rx_buffer.base, uart3_rx_buffer.size );

	// uart_printf( &huart3, &uart3_tx_buffer, "\nstart\n" ); todo debug line != data-line messup stuff otherwise
	while( 1 )
	{
		__HAL_UART_ENABLE_IT( &huart3, UART_IT_CM );

		if( uart3_cmd_CR_recvd == 0 && stream_mode == 0 )
		{
			cpu_enter_sleep_mode();
		}

		// redundant if not in stream-mode
		__HAL_UART_DISABLE_IT( &huart3, UART_IT_CM );
		
		// check if we received a usr command
		parse_external_command( uart3_rx_buffer.base, uart3_rx_buffer.size );

		usart3_receive_handler();

		switch( usrcmd ) {
		case USR_CMD_SINGLE_MEASURE_START:
			micro_spec_init();

			err = micro_spec_measure_start();
			if( err ) break;

			micro_spec_deinit();

			send_data( data_format );
			break;

		case USR_CMD_WRITE_ITIME:
			micro_spec_set_integration_time( usr_cmd_data );
			break;

		case USR_CMD_READ_ITIME:
			if( data_format == DATA_FORMAT_BIN )
			{
				HAL_UART_Transmit( &huart3, (uint8_t *) &hms1.integrtion_time, 4, 1000 );
			}
			else
			{
				uart_printf( &huart3, &uart3_tx_buffer, "integration time = %ld us\n", hms1.integrtion_time );
			}
			break;

		case USR_CMD_STREAM_START:
			micro_spec_init();
			stream_mode = 1;
			break;

		case USR_CMD_STREAM_END:
			micro_spec_deinit();
			stream_mode = 0;
			break;

		case USR_CMD_SET_FORMAT:
			data_format = (usr_cmd_data > 0) ? DATA_FORMAT_ASCII : DATA_FORMAT_BIN;
			break;

		default:
			break;
		}

		if( stream_mode )
		{

			err = micro_spec_measure_start();
			if( err ) break;
			send_data( data_format );
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
 * \param format	0 (DATA_FORMAT_BIN) send raw data, byte per byte.(eg. (dez) 1000 -> 0xE8 0x03)\n
 * \param format	1 (DATA_FORMAT_ASCII) send the data as in ASCII, as human readable text. (eg. (dez) 1000 -> '1' '0' '0' '0')
 *
 *
 */
static void send_data( uint8_t format )
{
	if( format == DATA_FORMAT_BIN )
	{
		//send data
		uint32_t no_err = ERRC_NO_ERROR;
		HAL_UART_Transmit( &huart3, (uint8_t *) &no_err, 2, 200 );
		HAL_UART_Transmit( &huart3, (uint8_t *) (hms1.data->wptr - MSPARAM_PIXEL), MSPARAM_PIXEL * 2, MSPARAM_PIXEL * 2 * 100 );
		
	}

	else if( format == DATA_FORMAT_ASCII )
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

	uint32_t errcode = ERRC_UNKNOWN;
	switch( err ) {
	case 0:
		return;
		
	case MS_ERR_TIMEOUT:
		if( data_format == DATA_FORMAT_ASCII )
		{
			uart_printf( &huart3, &uart3_tx_buffer, "ERR: TIMEOUT. Please check the following:\n"
				"1. is sensor plugged ?\n"
				"2. ADC/sensor powered ?\n"
				"3. check physical connections\n" );
		}
		errcode = ERRC_TIMEOUT;
		stream_mode = 0;
		micro_spec_deinit();
		break;
		
	case MS_ERR_NO_EOS:
		if( data_format == DATA_FORMAT_ASCII )
		{
			uart_printf( &huart3, &uart3_tx_buffer, "ERR: NO EOS. Something went wrong, please debug manually.\n" );
		}

		errcode = ERRC_NO_EOS;
		stream_mode = 0;
		micro_spec_deinit();
		break;

	case MS_ERR_EOS_EARLY:
		if( data_format == DATA_FORMAT_ASCII )
		{
			uart_printf( &huart3, &uart3_tx_buffer, "ERR: EOS EARLY. Something went wrong, please debug manually.\n" );
		}

		errcode = ERRC_EOS_EARLY;
		stream_mode = 0;
		micro_spec_deinit();
		break;
		
	default:
		break;
	}

	// send error code
	if( data_format == DATA_FORMAT_BIN )
	{
		HAL_UART_Transmit( &huart3, (uint8_t *) &errcode, 2, 200 );
	}
}

/**
 * @brief cpu_sleep()
 * Disabel SysTick and make CPU enter sleep-mode.
 *
 * @sa cpu_awake()
 */
void cpu_enter_sleep_mode( void )
{
	
	// to prevent wakeup by Systick interrupt.
	HAL_SuspendTick();
	// go back to sleep after handling an IR
	//HAL_PWR_EnableSleepOnExit(); TODO TEST
	HAL_PWR_EnterSLEEPMode( PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI );
}

/**
 * @brief cpu_awake()
 * EXECUTED BY ISR - KEEP IT SHORT
 */
void cpu_enter_run_mode( void )
{
	// wake up after handling the actual IR
	CLEAR_BIT( SCB->SCR, ((uint32_t)SCB_SCR_SLEEPONEXIT_Msk) );
	HAL_ResumeTick();
}

void parse_external_command( uint8_t *buffer, uint16_t size )
{
	UNUSED( size );
	usrcmd = USR_CMD_UNKNOWN;
	char *str, *alias;
	uint16_t sz, aliassz;

	str = "format=";
	sz = strlen( str );
	if( memcmp( buffer, str, sz ) == 0 )
	{
		sscanf( (char*) (buffer + sz), "%lu", &usr_cmd_data );
		usrcmd = USR_CMD_SET_FORMAT;
		return;
	}

	str = "measure\r";
	alias = "m\r";
	sz = strlen( str );
	aliassz = strlen( alias );
	if( memcmp( buffer, str, sz ) == 0 || memcmp( buffer, alias, aliassz ) == 0 )
	{
		usrcmd = USR_CMD_SINGLE_MEASURE_START;
		return;
	}

	str = "stream\r";
	sz = strlen( str );
	if( memcmp( buffer, str, sz ) == 0 )
	{
		usrcmd = USR_CMD_STREAM_START;
		return;
	}

	str = "end\r";
	sz = strlen( str );
	if( memcmp( buffer, str, sz ) == 0 )
	{
		usrcmd = USR_CMD_STREAM_END;
		return;
	}

	str = "itime=";
	alias = "i=";
	sz = strlen( str );
	aliassz = strlen( alias );
	if( memcmp( buffer, str, sz ) == 0 || memcmp( buffer, alias, aliassz ) == 0 )
	{
		// search the '=', than parse the value
		str = memchr( buffer, '=', sz );
		sscanf( str + 1, "%lu", &usr_cmd_data );
		usrcmd = USR_CMD_WRITE_ITIME;
		return;
	}

	str = "itime?\r";
	alias = "i?\r";
	sz = strlen( str );
	aliassz = strlen( alias );
	if( memcmp( buffer, str, sz ) == 0 || memcmp( buffer, alias, aliassz ) == 0 )
	{
		usrcmd = USR_CMD_READ_ITIME;
		return;
	}
}

