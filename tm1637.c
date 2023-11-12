/**
 * Copyright (c) 2017-2018, Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 *
 * mods (c) TDSystem Thomas Dausner 2021
 *
 * This is ATtiny13/25/45/85 library for 4-Digit LED Display based on TM1637 chip.
 *
 * Features:
 * - display raw segments
 * - display digits
 * - display colon
 * - display on/off
 * - brightness control
 *
 * References:
 * - library: https://github.com/lpodkalicki/attiny-tm1637-library
 * - documentation: https://github.com/lpodkalicki/attiny-tm1637-library/README.md
 * - TM1637 datasheet: https://github.com/lpodkalicki/attiny-tm1637-library/blob/master/docs/TM1637_V2.4_EN.pdf
 */
// #define F_CPU 1E6	// 1,0 MHz - set in tool chain
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "tm1637.h"

#define	TM1637_DIO_HIGH()		(PORTB |= _BV(TM1637_DIO_PIN))
#define	TM1637_DIO_LOW()		(PORTB &= ~_BV(TM1637_DIO_PIN))
#define	TM1637_DIO_OUTPUT()		(DDRB |= _BV(TM1637_DIO_PIN))
#define	TM1637_DIO_INPUT()		(DDRB &= ~_BV(TM1637_DIO_PIN))
#define	TM1637_DIO_READ() 		(((PINB & _BV(TM1637_DIO_PIN)) > 0) ? 1 : 0)
#define	TM1637_CLK_HIGH()		(PORTB |= _BV(TM1637_CLK_PIN))
#define	TM1637_CLK_LOW()		(PORTB &= ~_BV(TM1637_CLK_PIN))

static void TM1637_send_config(const uint8_t enable, const uint8_t brightness);
static void TM1637_send_command(const uint8_t value);
static void TM1637_start(void);
static void TM1637_stop(void);
static uint8_t TM1637_write_byte(uint8_t value);

static uint8_t _config = TM1637_SET_DISPLAY_ON | TM1637_BRIGHTNESS_MAX;
static uint8_t _segments = 0xff;
PROGMEM const uint8_t _digit2segments[] =
{
/*
 *    bits:
 *   -- 0 --
 *  |       |
 *  5       1
 *  |       |
 *   -- 6 --
 *  |       |
 *  4       2
 *  |       |
 *   -- 3 --
 */
	0x3F, // 0
	0x06, // 1
	0x5B, // 2
	0x4F, // 3
	0x66, // 4
	0x6D, // 5
	0x7D, // 6
	0x07, // 7
	0x7F, // 8
	0x6F, // 9	
    0x77, // A
    0x7C, // b
    0x39, // C
    0x5E, // d
    0x79, // E
    0x71, // F

    0x76, // H
    0x10, // i
    0x38, // L
    0x54, // n
    0x5C, // o
    0x73, // P
	0x50, // r
	0x78, // t
    0x3E, // U
    0x1C, // u
	0x40, // -
	0x08  // _
};
#define NUM_LETTERS (sizeof(_digit2segments) / sizeof(uint8_t))

void 
TM1637_init(const uint8_t enable, const uint8_t brightness)
{

	DDRB |= (_BV(TM1637_DIO_PIN)|_BV(TM1637_CLK_PIN));
	PORTB &= ~(_BV(TM1637_DIO_PIN)|_BV(TM1637_CLK_PIN));
	TM1637_send_config(enable, brightness);
}

void
TM1637_enable(const uint8_t value)
{

	TM1637_send_config(value, _config & TM1637_BRIGHTNESS_MAX);
}

void
TM1637_set_brightness(const uint8_t value)
{

	TM1637_send_config(_config & TM1637_SET_DISPLAY_ON,
		value & TM1637_BRIGHTNESS_MAX);
}

void
TM1637_display_segments(const uint8_t position, const uint8_t segments)
{

	TM1637_send_command(TM1637_CMD_SET_DATA | TM1637_SET_DATA_F_ADDR);
	TM1637_start();
	TM1637_write_byte(TM1637_CMD_SET_ADDR | (position & (TM1637_POSITION_MAX - 1)));
	TM1637_write_byte(segments);
	TM1637_stop();
}

void
TM1637_display_digit(const uint8_t position, const uint8_t digit)
{
	uint8_t segments = (digit < NUM_LETTERS ? pgm_read_byte_near((uint8_t *)&_digit2segments + digit) : 0x00);

	if (position == 0x01) {
		segments = segments | (_segments & 0x80);
		_segments = segments;
	}

	TM1637_display_segments(position, segments);
}

void
TM1637_display_msg(const uint8_t msg[])
{
	for (uint8_t pos = 0; pos < TM1637_POSITION_MAX; pos++)
	{
		TM1637_display_digit(pos, *(msg + pos));
	}
}

void
TM1637_display_colon(const uint8_t value)
{

	if (value) {
		_segments |= 0x80;
	} else {
		_segments &= ~0x80;
	}
	TM1637_display_segments(0x01, _segments);
}

void
TM1637_clear(void)
{
	uint8_t i;

	_segments = 0;
	for (i = 0; i < TM1637_POSITION_MAX; ++i) {
		TM1637_display_segments(i, 0x00);
	}
}

void
TM1637_send_config(const uint8_t enable, const uint8_t brightness)
{

	_config = (enable ? TM1637_SET_DISPLAY_ON : TM1637_SET_DISPLAY_OFF) |
		(brightness > TM1637_BRIGHTNESS_MAX ? TM1637_BRIGHTNESS_MAX : brightness);

	TM1637_send_command(TM1637_CMD_SET_DSIPLAY | _config);
}

void
TM1637_send_command(const uint8_t value)
{

	TM1637_start();
	TM1637_write_byte(value);
	TM1637_stop();
}

void
TM1637_start(void)
{

	TM1637_DIO_HIGH();
	TM1637_CLK_HIGH();
	_delay_us(TM1637_DELAY_US);
	TM1637_DIO_LOW();
}

void
TM1637_stop(void)
{

	TM1637_CLK_LOW();
	_delay_us(TM1637_DELAY_US);

	TM1637_DIO_LOW();
	_delay_us(TM1637_DELAY_US);

	TM1637_CLK_HIGH();
	_delay_us(TM1637_DELAY_US);

	TM1637_DIO_HIGH();
}

uint8_t TM1637_ack()
{
	uint8_t ack;
	
	TM1637_CLK_LOW();
	TM1637_DIO_INPUT();
	TM1637_DIO_HIGH();
	_delay_us(TM1637_DELAY_US);

	ack = TM1637_DIO_READ();
	if (ack) {
		TM1637_DIO_OUTPUT();
		TM1637_DIO_LOW();
	}
	_delay_us(TM1637_DELAY_US);

	TM1637_CLK_HIGH();
	_delay_us(TM1637_DELAY_US);

	TM1637_CLK_LOW();
	_delay_us(TM1637_DELAY_US);

	TM1637_DIO_OUTPUT();

	return ack;
}

uint8_t
TM1637_write_byte(uint8_t value)
{
	uint8_t i;

	for (i = 0; i < 8; ++i, value >>= 1) {
		TM1637_CLK_LOW();
		_delay_us(TM1637_DELAY_US);

		if (value & 0x01) {
			TM1637_DIO_HIGH();
		} else {
			TM1637_DIO_LOW();
		}

		TM1637_CLK_HIGH();
		_delay_us(TM1637_DELAY_US);
	}
	return TM1637_ack();
}

uint8_t
TM1637_keyscan()
{
	uint8_t i;
	uint8_t keys = 0;

	TM1637_start();
	TM1637_write_byte(TM1637_CMD_SET_DATA | TM1637_SET_DATA_READ); // command: read status of keys
	TM1637_DIO_INPUT();
	for (i = 0; i < 8; i++)	{
		TM1637_CLK_LOW();
		keys >>= 1;
		_delay_us(TM1637_DELAY_US);
		TM1637_CLK_HIGH();
		if (TM1637_DIO_READ()) {
			keys |= 0x80;
		}
		_delay_us(TM1637_DELAY_US);
	}
	TM1637_DIO_OUTPUT();
	TM1637_ack();
	TM1637_stop();
	return keys;
}
