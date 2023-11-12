/*
 * uart.c
 *
 * Created: 19.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 *
 * simple serial transfer library
 */ 

// #define F_CPU 1E6	// 1,0 MHz - set in tool chain
#include <stddef.h>
#include <avr/io.h>
#include <avr/common.h>
#include <util/delay.h>
#include "uart.h"

/**
 * transfer byte
 *
 * transfer is at ~19.200 Baud / 52,1[us] bit time
 *
 * the bit banging algorithm reads in port / sets level / writes out port
 * to keep symmetric hi and lo bits
 */
void uart_tx(register char data)
{
	register uint8_t bit = _BV(0);
	register uint8_t pb;

	UART_TXDRR |= _BV(UART_TXBIT);		// out
	UART_TXPORT &= ~_BV(UART_TXBIT);	// start bit
	_delay_us(42);
	while (bit) {
		pb = UART_TXPORT;
		if (data & bit) 
			 pb |= _BV(UART_TXBIT);
		else pb &= ~_BV(UART_TXBIT);
		UART_TXPORT = pb;
		_delay_us(41);
		bit <<= 1;
	}
	_delay_us(7);	// compensation for end of while - last bit
	
	UART_TXPORT |= _BV(UART_TXBIT);	// stop bit
	_delay_us(40);
	UART_TXDRR &= ~_BV(UART_TXBIT);	// in
}

/**
 * transmit zero terminated string
 */
void uart_tx_string(char *s)
{
	do {
		uart_tx(*s++);
	} while (*s != 0);
}
