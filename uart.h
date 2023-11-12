/*
 * uart.h
 *
 * Created: 19.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 */ 

#ifndef UART_H_
#define UART_H_

#define UART_TXDRR	DDRB
#define UART_TXPORT	PORTB
#define UART_TXBIT	PB3
/**
 * no RX implemented
 */

void uart_tx(char data);
void uart_tx_string(char *s);

#endif /* UART_H_ */