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

/* Defines for printf support. @sa _write() */
#define STDOUT_FILENO   1
#define STDERR_FILENO   2
#define hprintf huart4

/* Define which uart interface to use for receiving
 * and transmitting via HAL_UART_Transmit() and
 * HAL_UART_Receive{_DMA,_IT}()*/
#define hrxtx huart4
#define RXTX UART4
#define RXTX_IRQn UART4_IRQn

// This should be as least as big that we can send one whole
// measurement data plus some meta data.
#define UART_TX_BUFFER_SZ	(1024)

// Small as we just need it for receiving user commands
#define UART_RX_BUFFER_SZ	(128)

/* used with HAL_UART_Receive() */
typedef struct {
	const uint16_t size; /* !< size in bytes. */
	uint8_t *base; /* !< pointer to the start of the buffer. */
} uart_buffer_t;

/* used with HAL_UART_Receive() */
extern uart_buffer_t rxtx_rxbuffer;

/* extern vars, see .c for info */
extern volatile bool rxtx_CR_recvd;
extern volatile uint16_t rxtx_cmd_bytes;
extern uint8_t tx_dbgflg;

int _write(int file, char *ptr, int len);
void rxtx_init(void);
void rxtx_restart_listening(void);
int uart_printf(UART_HandleTypeDef *uart_handle, uart_buffer_t *tx_buffer, const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 3, 4)) );
int debug(const char *__restrict format, ...)__attribute__( (__format__ (__printf__, 1, 2)) );

#endif /*__usart_usr_H */
