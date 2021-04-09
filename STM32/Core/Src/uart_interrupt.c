/*
 * uart_interrupt.c
 *
 *  Created on: 8 gru 2020
 *      Author: Karol
 */

#include "uart_interrupt.h"

#define myUART USART2

uint8_t uart_buffer[MAX_LENGTH];

uint16_t data_length=0;

UART_STATUS_t uart_state;

uint8_t trash;


__weak void uartRxClb(uint8_t data){
	uart_buffer[data_length] =
	data_length++;
	uart_state = UART_NOT_READY;
}

__weak void uartRxIdle(){
	uart_state = UART_READY;
}


void uart_interrupt_isr(){

	if( myUART->SR & USART_SR_RXNE ){
		uint8_t data = myUART->DR;
		uartRxClb(data);

	}
	if( myUART->SR & USART_SR_ORE){
		trash = myUART->DR;
	}

	/*
	else if( USART2->SR & USART_SR_IDLE){
		USART2->CR1 &= ~USART_CR1_IDLEIE;
		trash = USART2->DR;
		uartRxIdle();
	}*/

}


void uart_send_msg( uint8_t *msg, uint8_t lng, uint16_t timeout ){

	uint8_t ctr;
	uint32_t statr_tick;

	for( ctr = 0; ctr < lng; ctr++){

		myUART->DR = msg[ctr];

		statr_tick = HAL_GetTick();

		while( !(myUART->SR & USART_SR_TXE) ){
			if( HAL_GetTick()- statr_tick > timeout ) return;
		}

	}

}


void uart_interrupts_enable(){
	data_length = 0;
	uart_state = UART_NOT_READY;
	myUART->CR1 |= USART_CR1_RXNEIE;
}


UART_STATUS_t uart_get_state(){
	return uart_state;
}
void uart_get_data( uint8_t *uart_data ){

	uint8_t ctr;
	for( ctr = 0; ctr < data_length; ctr++){
		uart_data[ctr] = uart_buffer[ctr];
	}
	data_length = 0;
	uart_state = UART_NOT_READY;

}
