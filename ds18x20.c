/*
 * DS18x20 lib
 *
 * based on ds18b20 lib 0x02 (copyright (c) Davide Gironi, 2012)
 * 
 * mods (c) TDSystem Thomas Dausner 2021
 * 
 * Released under GPLv3.
 * Please refer to LICENSE file for licensing information.
 */

// #define F_CPU 1E6	// 1,0 MHz - set in tool chain
#include <avr/io.h>
#include <avr/common.h>
#include <util/delay.h>
#include "ds18x20.h"

/*
 * DS18x20 init
 */
uint8_t DS18x20_reset()
{
	uint8_t rc;
	
	DS18x20_LOW();
	DS18x20_OUTPUT();
	_delay_us(550);
	DS18x20_INPUT();
	_delay_us(80);

	rc = DS18x20_READ();
	_delay_us(550);

	return rc;	// 0 = ok, 1 = error
}

/*
 * write single bit
 */
void DS18x20_writebit(uint8_t bit)
{
	DS18x20_LOW();
	DS18x20_OUTPUT();
	_delay_us(1);

	if (bit)
		DS18x20_INPUT();

	_delay_us(60);
	DS18x20_INPUT();
}

/*
 * read single bit
 */
uint8_t DS18x20_readbit(void)
{
	uint8_t bit = 0;

	DS18x20_LOW();
	DS18x20_OUTPUT();
	_delay_us(1);
	DS18x20_INPUT();
	_delay_us(14);

	bit = DS18x20_READ();
	_delay_us(45);
	return bit;
}

/*
 * write byte
 */
void DS18x20_writebyte(uint8_t byte)
{
	uint8_t i = 8;
	
	while (i--) {
		DS18x20_writebit(byte & 0x01);
		byte >>= 1;
	}
}

/*
 * read byte
 */
uint8_t DS18x20_readbyte(void)
{
	uint8_t i = 8, byte = 0;
	
	while (i--) {
		byte = (byte >> 1) | (DS18x20_readbit() << 7);
	}
	return byte;
}

/*
 * start conversion - async operation
 *
 * returns
 *   DS18x20_NO_RESET - error no sensor reset
 *   DS18x20_NO_VAL   - ok, no value
 */
int16_t DS18x20_startcv()
{
	uint16_t temperature = DS18x20_NO_RESET;

	if (DS18x20_reset() == 0) {

#if DS18x20_RES > 0
		DS18x20_writebyte(DS18x20_CMD_WSCRATCHPAD);
		DS18x20_writebyte(0);	// TH user byte 1
		DS18x20_writebyte(0);	// TL user byte 2
		DS18x20_writebyte(DS18x20_RES);
		DS18x20_reset();
#endif // DS18x20_RES

		DS18x20_writebyte(DS18x20_CMD_SKIPROM);
		DS18x20_writebyte(DS18x20_CMD_CONVERTTEMP);
		temperature = DS18x20_NO_VALUE;
		DS18x20_PWRON();
	}
	return temperature;
}

/*
 * read temperature - async operation
 *
 * returns
 *   DS18x20_NO_DATA - error no sensor data
 *   0x07D0...0xFC90  ~  +125[°]...-55[°] on 12 Bit resolution
 *   0x00AA...0xFF92  ~   +85[°]...-55[°] in  9 Bit resolution
 */
int16_t DS18x20_readtemp()
{
	uint8_t temperature_l;
	uint16_t temperature = DS18x20_NO_DATA;
	
	DS18x20_PWROFF();

	if (DS18x20_readbit()) {	// check conversion complete
		DS18x20_reset();
		DS18x20_writebyte(DS18x20_CMD_SKIPROM);
		DS18x20_writebyte(DS18x20_CMD_RSCRATCHPAD);

		//read 2 byte from scratch pad
		temperature_l = DS18x20_readbyte();
		temperature = (DS18x20_readbyte() << 8) + temperature_l;
	}
	return temperature;
}

/*
 * get temperature - sync operation
 */
/* no sync operation
#include <avr/interrupt.h>
int16_t DS18x20_gettemp()
{
	uint8_t interrupt;
	uint16_t temperature;
	
	interrupt = (SREG & _BV(SREG_I));
	if (interrupt) cli();

	temperature = DS18x20_startcv();
	if (temperature != DS18x20_NO_RESET) {

		if (interrupt) sei();
		_delay_ms(DS18x20_CVT);
		DS18x20_PWROFF();
		if (interrupt) cli();
		
		temperature = DS18x20_readtemp();
	}
	if (interrupt) sei(); 
	
	return temperature;
}
*/