/**
  ******************************************************************************
  * File Name          : USART.h
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __usart_usr_H
#define __usart_usr_H

/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "global_config.h"

// configure printf
#define STDOUT_FILENO   1
#define STDERR_FILENO   2
#define hprintf huart1
int _write(int file, char *ptr, int len);

// configure the usr comunication interface
#define hrxtx huart1
#define RXTX USART1
#define RXTX_IRQn USART1_IRQn

// this should be as least as big that we can send one whole
// measurement data plus some meta data.
#define UART_DEFAULT_TX_BUFFER_SZ	(1024)

// small as we just need it for receiving user commands
#define UART_DEFAULT_RX_BUFFER_SZ	(128)

typedef struct
{
	const uint16_t size; /* !< size in bytes. */
	uint8_t *base; /* !< pointer to the start of the buffer. */
} uart_buffer_t;

extern uart_buffer_t rxtx_rxbuffer;
extern uart_buffer_t rxtx_txbuffer;

extern volatile bool rxtx_CR_recvd;
extern volatile uint16_t rxtx_cmd_bytes;

void rxtx_init( void );
void rx_handler( void );
int uart_printf( UART_HandleTypeDef *uart_handle, uart_buffer_t *tx_buffer, const char *__restrict format, ... )__attribute__( (__format__ (__printf__, 3, 4)) );

int tx_printf( const char *__restrict format, ... )__attribute__( (__format__ (__printf__, 1, 2)) );

#endif /*__usart_usr_H */
