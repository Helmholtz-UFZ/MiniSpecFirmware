/*
 * main_usr.c
 *
 *  Created on: Aug 7, 2017
 *      Author: Bert Palm
 */

#include "main_usr.h"
#include "rtc.h"
#include "rtc_usr.h"
#include "global_config.h"
#include "micro_spec.h"
#include "usart_usr.h"
#include "tim_usr.h"
#include "sd_card.h"
#include "string.h"
#include <stdio.h>

static void send_data( uint8_t format );
static void error_handler( uint8_t err );
static void testtest(void);

static void parse_extcmd( uint8_t *buffer, uint16_t size );
static usr_cmd_typedef extcmd;

static uint8_t data_format = DATA_FORMAT_ASCII;
static bool stream_mode = 0;

static RTC_DateTypeDef sDate;
static RTC_TimeTypeDef sTime;
static RTC_TimeTypeDef tmp;

int main_usr( void )
{
	uint8_t err = 0;
	uint32_t tmp;
	char *str;
	uint8_t c;


	/* Pre init - undo CubeMX stuff ---------------------------------------------*/

	// we enable IRs where/when ** WE ** need them.
	HAL_NVIC_DisableIRQ( RXTX_IRQn );
	HAL_NVIC_DisableIRQ( TIM1_CC_IRQn );
	HAL_NVIC_DisableIRQ( EXTI2_IRQn );

	rxtx_init();
	tim1_Init();
	tim2_Init();
	tim5_Init();

	/* Run the system ------------------------------------------------------------*/

	// enabling usart receiving
	NVIC_EnableIRQ( RXTX_IRQn );
	HAL_UART_Receive_DMA( &hrxtx, rxtx_rxbuffer.base, rxtx_rxbuffer.size );

	if( data_format == DATA_FORMAT_ASCII )
	{
		tx_printf( "\nstart\n" );
	}
	while( 1 )
	{
		// IR in uart module
		__HAL_UART_ENABLE_IT( &hrxtx, UART_IT_CM );

		if( rxtx_CR_recvd == 0 && stream_mode == 0 )
		{
			cpu_enter_sleep_mode();
		}

		// redundant in non-stream mode as also disabled in its ISR
		__HAL_UART_DISABLE_IT( &hrxtx, UART_IT_CM );
		
		parse_extcmd( rxtx_rxbuffer.base, rxtx_rxbuffer.size );

		rx_handler();

		switch( extcmd.cmd ) {
		case USR_CMD_SINGLE_MEASURE_START:

			if(data_format == DATA_FORMAT_ASCII)
				tx_printf( "ok\n" );

			sensor_init();

			err = sensor_measure();
			if( err ) break;

			sensor_deinit();

			send_data( data_format );
			break;

		case USR_CMD_WRITE_ITIME:

			/* parse argument */
			if (extcmd.arg_buffer[0] == 0) {
				break;
			}else{
				sscanf(extcmd.arg_buffer, "%lu", &tmp);
			}

			/* check and set argument */
			sensor_set_itime( tmp );

			if(data_format == DATA_FORMAT_ASCII)
				tx_printf( "ok\n" );

			break;

		case USR_CMD_READ_ITIME:
			if( data_format == DATA_FORMAT_BIN )
			{
				HAL_UART_Transmit( &hrxtx, (uint8_t *) &sens1.itime, 4, 1000 );
			}
			else
			{
				tx_printf( "integration time = %ld us\n", sens1.itime );
			}
			break;

		case USR_CMD_STREAM_START:

			if(data_format == DATA_FORMAT_ASCII)
				tx_printf( "ok\n" );

			sensor_init();
			stream_mode = 1;
			break;

		case USR_CMD_STREAM_END:
			sensor_deinit();
			stream_mode = 0;

			if(data_format == DATA_FORMAT_ASCII)
				tx_printf( "ok\n" );

			break;

		case USR_CMD_SET_FORMAT:
			/* parse argument */
			if (extcmd.arg_buffer[0] == 0) {
				tmp = 1;
			}else{
				sscanf(extcmd.arg_buffer, "%lu", &tmp);
			}

			/* check and set argument */
			data_format = (tmp > 0) ? DATA_FORMAT_ASCII : DATA_FORMAT_BIN;

			if(data_format == DATA_FORMAT_ASCII)
				tx_printf( "ok\n" );

			break;

		case USR_CMD_DEBUG:
			testtest();
			break;

		case USR_CMD_GET_RTC_TIME:
			/* Always call GetDate after GetTime ! see HAL-documentation */
			HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

			if(data_format == DATA_FORMAT_ASCII){
				printf("20%i-%02i-%02iT%02i:%02i:%02i\n", sDate.Year, sDate.Month, sDate.Date,
						sTime.Hours, sTime.Minutes, sTime.Seconds);
			}else{
				/* Transmit binary */
				HAL_UART_Transmit( &hrxtx, (uint8_t *) &sDate.Year, 1, 1000 );
				HAL_UART_Transmit( &hrxtx, (uint8_t *) &sDate.Month, 1, 1000 );
				HAL_UART_Transmit( &hrxtx, (uint8_t *) &sDate.Date, 1, 1000 );
				HAL_UART_Transmit( &hrxtx, (uint8_t *) &sTime.Hours, 1, 1000 );
				HAL_UART_Transmit( &hrxtx, (uint8_t *) &sTime.Minutes, 1, 1000 );
				HAL_UART_Transmit( &hrxtx, (uint8_t *) &sTime.Seconds, 1, 1000 );
			}
			break;

		case USR_CMD_SET_RTC_TIME:

			if (extcmd.arg_buffer[0] == 0) {
				break;
			} else {
				str = extcmd.arg_buffer;
			}

			err = rtc_parse_datetime(str, &sTime, &sDate);
			if (err){
				break;
			}

			HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
			HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

			if(data_format == DATA_FORMAT_ASCII)
				tx_printf( "ok\n" );

			break;

		case USR_CMD_GET_INTERVAL:

			if(data_format == DATA_FORMAT_ASCII){
				printf("%02i:%02i:%02i\n", rtc_ival.Hours, rtc_ival.Minutes, rtc_ival.Seconds);

			}else{
				/* Transmit binary */
				HAL_UART_Transmit( &hrxtx, (uint8_t *) &rtc_ival.Hours, 1, 1000 );
				HAL_UART_Transmit( &hrxtx, (uint8_t *) &rtc_ival.Minutes, 1, 1000 );
				HAL_UART_Transmit( &hrxtx, (uint8_t *) &rtc_ival.Seconds, 1, 1000 );
			}
			break;

		case USR_CMD_SET_INTERVAL:
			if (extcmd.arg_buffer[0] == 0) {
				break;
			} else {
				str = extcmd.arg_buffer;
			}

			err = rtc_parse_interval(str, &sTime);
			if (err){
				break;
			}

			if( rtc_ival.Hours == 0 && rtc_ival.Minutes == 0 && rtc_ival.Seconds < MIN_IVAL ){
				break;
			}

			/* if all ok update the ival */
			rtc_ival.Hours = sTime.Hours;
			rtc_ival.Minutes = sTime.Minutes;
			rtc_ival.Seconds = sTime.Seconds;

			/*and set the alarm*/
			// todo

			if(data_format == DATA_FORMAT_ASCII)
				tx_printf( "ok\n" );

			break;

		default:
			break;
		}

		if( stream_mode )
		{
			err = sensor_measure();
			if( err == 0 )
			{
				send_data( data_format );
			}
		}

		if( err )
		{
			error_handler( sens1.status );
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
 */
static void send_data( uint8_t format )
{
	if( format == DATA_FORMAT_BIN )
	{
		//send data
		uint32_t no_err = ERRC_NO_ERROR;
		HAL_UART_Transmit( &hrxtx, (uint8_t *) &no_err, 2, 200 );
		HAL_UART_Transmit( &hrxtx, (uint8_t *) (sens1.data->wptr - MSPARAM_PIXEL), MSPARAM_PIXEL * 2, MSPARAM_PIXEL * 2 * 100 );
		
	}

	else if( format == DATA_FORMAT_ASCII )
	{
		uint16_t *rptr = sens1.data->base;
		uint16_t i = 0;
		
		while( rptr < sens1.data->wptr )
		{
			if( DBG_SEND_ALL == OFF && rptr < (sens1.data->wptr - MSPARAM_PIXEL) )
			{
				rptr++;
				continue;
			}
			
			if( rptr == (sens1.data->wptr - MSPARAM_PIXEL) )
			{
				tx_printf( "\n"DELIMITER_STR );
				tx_printf( "\n"HEADER_STR );
				i = 0;
			}
			
			if( i % 10 == 0 )
			{
				tx_printf( "\n%03d   %05d ", i, *rptr );
			}
			else
			{
				tx_printf( "%05d ", *rptr );
			}
			
			rptr++;
			i++;
		}
		tx_printf( "\n"DELIMITER_STR"\n\n" );
	}
	
}

/**
 * Handles error.
 * So far only errors from a measurement.
 */
static void error_handler( uint8_t err )
{

	uint32_t errcode = ERRC_UNKNOWN;
	switch( err ) {
	case 0:
		return;
		
	case SENS_ERR_TIMEOUT:
		if( data_format == DATA_FORMAT_ASCII )
		{
			tx_printf( "ERR: TIMEOUT. Please check the following:\n"
				"1. is sensor plugged ?\n"
				"2. ADC/sensor powered ?\n"
				"3. check physical connections\n" );
		}
		errcode = ERRC_TIMEOUT;
		stream_mode = 0;
		sensor_deinit();
		break;
		
	case SENS_ERR_NO_EOS:
		if( data_format == DATA_FORMAT_ASCII )
		{
			tx_printf( "ERR: NO EOS. Something went wrong, please debug manually.\n" );
		}

		errcode = ERRC_NO_EOS;
		stream_mode = 0;
		sensor_deinit();
		break;

	case SENS_ERR_EOS_EARLY:
		if( data_format == DATA_FORMAT_ASCII )
		{
			tx_printf( "ERR: EOS EARLY. Something went wrong, please debug manually.\n" );
		}

		errcode = ERRC_EOS_EARLY;
		stream_mode = 0;
		sensor_deinit();
		break;
		
	default:
		break;
	}

	// send error code
	if( data_format == DATA_FORMAT_BIN )
	{
		HAL_UART_Transmit( &hrxtx, (uint8_t *) &errcode, 2, 200 );
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
	// cleared by calling cpu_enter_run_mode() from ISR
	HAL_PWR_EnableSleepOnExit();

	//sleep + WaintForInterrupt-----------------------------------------
	HAL_PWR_EnterSLEEPMode( PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI );
	//awake again ------------------------------------------------------

	HAL_ResumeTick();
}

/**
 * @brief cpu_awake()
 * EXECUTED BY ISR - KEEP IT SHORT
 */
void cpu_enter_run_mode( void )
{
	// wake up after handling the actual IR
	CLEAR_BIT( SCB->SCR, ((uint32_t)SCB_SCR_SLEEPONEXIT_Msk) );
}


/*
 * check and parse for an command and set the
 * variable usrcmd if any command was found.
 *
 * The command have to be in the beginning
 * of the given buffer.
 *
 * @param buffer the buffer to check
 * @param size unused
 *
 * todo (future release) check the whole buffer up to size
 */
static void parse_extcmd( uint8_t *buffer, uint16_t size )
{
	UNUSED( size );
	char *str, *alias;
	uint16_t sz, aliassz;

	/* init cmd structure */
	extcmd.cmd = USR_CMD_UNKNOWN;
	memset(&extcmd.arg_buffer, 0, ARGBUFFSZ);

	str = "measure\r";
	alias = "m\r";
	sz = strlen( str );
	aliassz = strlen( alias );
	if( memcmp( buffer, str, sz ) == 0 || memcmp( buffer, alias, aliassz ) == 0 )
	{
		extcmd.cmd = USR_CMD_SINGLE_MEASURE_START;
		return;
	}

	str = "stream\r";
	sz = strlen( str );
	if( memcmp( buffer, str, sz ) == 0 )
	{
		extcmd.cmd = USR_CMD_STREAM_START;
		return;
	}

	str = "end\r";
	sz = strlen( str );
	if( memcmp( buffer, str, sz ) == 0 )
	{
		extcmd.cmd = USR_CMD_STREAM_END;
		return;
	}

	str = "debug\r";
	alias = "d\r";
	sz = strlen( str );
	aliassz = strlen( alias );
	if( memcmp( buffer, str, sz ) == 0 || memcmp( buffer, alias, aliassz ) == 0 )
	{
		extcmd.cmd = USR_CMD_DEBUG;
		return;
	}

	str = "rtc?\r";
	sz = strlen( str );
	if( memcmp( buffer, str, sz ) == 0 )
	{
		extcmd.cmd = USR_CMD_GET_RTC_TIME;
		return;
	}

	str = "ival?\r";
	sz = strlen( str );
	if( memcmp( buffer, str, sz ) == 0 )
	{
		extcmd.cmd = USR_CMD_GET_INTERVAL;
		return;
	}

	str = "itime?\r";
	alias = "i?\r";
	sz = strlen( str );
	aliassz = strlen( alias );
	if( memcmp( buffer, str, sz ) == 0 || memcmp( buffer, alias, aliassz ) == 0 )
	{
		extcmd.cmd = USR_CMD_READ_ITIME;
		return;
	}

	str = "itime=";
	alias = "i=";
	sz = strlen( str );
	aliassz = strlen( alias );
	if( memcmp( buffer, str, sz ) == 0 || memcmp( buffer, alias, aliassz ) == 0 )
	{
		extcmd.cmd = USR_CMD_WRITE_ITIME;
		/* Set pointer to char after the '=' */
		str = (char*) memchr( buffer, '=', sz ) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "format=";
	sz = strlen( str );
	if( memcmp( buffer, str, sz ) == 0 )
	{
		extcmd.cmd = USR_CMD_SET_FORMAT;
		/* Set pointer to char after the '=' */
		str = (char*) memchr( buffer, '=', sz ) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);
		return;
	}

	str = "rtc=";
	sz = strlen( str );
	if( memcmp( buffer, str, sz ) == 0 )
	{

		extcmd.cmd = USR_CMD_SET_RTC_TIME;
		/* Set pointer to char after the '=' */
		str = (char*) memchr( buffer, '=', sz ) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);

		return;
	}

	str = "ival=";
	sz = strlen( str );
	if( memcmp( buffer, str, sz ) == 0 )
	{

		extcmd.cmd = USR_CMD_SET_INTERVAL;
		/* Set pointer to char after the '=' */
		str = (char*) memchr( buffer, '=', sz ) + 1;
		/* Copy arg str to arg_buffer, so we can reset the receive buffer and
		 * listening again on the rx line. */
		strncpy(extcmd.arg_buffer, str, ARGBUFFSZ);

		return;
	}
}


static void testtest(void){
	uint8_t res = 0;
//	res = sd_format();
//	printf("format: %i\n", res);
	res = sd_mount();
	printf("mount: %i\n", res);
	res = sd_write_file("F1.TXT", "some in line1\r\nline2\r\n");
	printf("first: %i\n", res);
	res = sd_write_file("F1.TXT", "more here");
	printf("sec: %i\n", res);
	res = sd_umount();
}
