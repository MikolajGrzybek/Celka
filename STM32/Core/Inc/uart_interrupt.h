/*
 * uart_interrupt.h
 *
 *  Created on: 8 gru 2020
 *      Author: Karol
 */

#ifndef INC_UART_INTERRUPT_H_
#define INC_UART_INTERRUPT_H_

#include "main.h"

#define MAX_LENGTH 20

typedef enum {UART_READY, UART_NOT_READY} UART_STATUS_t;

void uart_interrupt_isr();

UART_STATUS_t uart_get_state();
void uart_get_data( uint8_t *uart_data );
void uart_interrupts_enable();
void uart_send_msg( uint8_t *msg, uint8_t lng, uint16_t timeout );



#endif /* INC_UART_INTERRUPT_H_ */
