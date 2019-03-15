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
#define hprintf huart4

// configure the usr comunication interface
#define hrxtx huart4
#define RXTX UART4
#define RXTX_IRQn UART4_IRQn

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

extern volatile bool rxtx_CR_recvd;
extern volatile uint16_t rxtx_cmd_bytes;

/* flag for switching on/off debugging printf during runtime */
extern uint8_t tx_dbgflg;

// printf support
int _write(int file, char *ptr, int len);
void rxtx_init( void );
void rx_handler( void );
int uart_printf( UART_HandleTypeDef *uart_handle, uart_buffer_t *tx_buffer, const char *__restrict format, ... )__attribute__( (__format__ (__printf__, 3, 4)) );

int tx_printf( const char *__restrict format, ... )__attribute__( (__format__ (__printf__, 1, 2)) );
int debug(const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 1, 2)) );

#endif /*__usart_usr_H */
