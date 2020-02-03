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
#include <globalconfig.h>
#include "usart.h"

/* Defines for printf support. @sa _write() */
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

/* Define which uart interface to use for receiving
 * and transmitting via HAL_UART_Transmit() and
 * HAL_UART_Receive{_DMA,_IT}()*/
#if UART_INSTANCE_NR == 4
# define hprintf huart4
# define hrxtx huart4
# define RXTX UART4
# define RXTX_IRQn UART4_IRQn
#elif UART_INSTANCE_NR == 1
# define hprintf huart1
# define hrxtx huart1
# define RXTX USART1
# define RXTX_IRQn USART1_IRQn
#else
#error "Not implemented choice of UART_INSTANCE_NR"
#endif /*UART_INSTANCE_NR*/

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

typedef struct {
	/* The number of bytes received */
	volatile uint16_t cmd_bytes;

} rxtx_t;

extern rxtx_t rxtx;

/* used with HAL_UART_Receive() */
extern uart_buffer_t rxtx_rxbuffer;

int _write(int file, char *ptr, int len);
void rxtx_init(void);
void rxtx_restart_listening(void);

#endif /*__usart_usr_H */
