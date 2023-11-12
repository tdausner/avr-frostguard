/*
 * mode_temp.c
 *
 * Created: 20.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 */
#include <stdint.h>
#include "ds18x20.h"
#include "tm1637.h"
#include "frostguard.h"
#include "globals.h"

/**
 * set threshold temperatures mode
 *
 * - show "H" in first digit and high temperature (blinking)
 * - KEY_UP/KEY_DOWN -> incr/decr high temperature (not blinking)
 * - KEY_SET -> show "L" in first digit and high low temperature (blinking)
 * - KEY_UP/KEY_DOWN -> incr/decr low temperature (not blinking)
 * - KEY_SET -> store high/low temperatures, display off and leave
 */
uint8_t mode_temperatures(uint8_t key)
{
	uint8_t	rc = MDS_RUN;
	uint8_t	dir;
	
	switch (globals.submode) {
		case 0:
			globals.dsp_stat = DSP_BLINK;
			displayTemp((int)globals.params.temperatures.high);
			TM1637_display_digit(0, _DSP_H);
			globals.submode++;
			break;
		case 1:
			if (key == KEY_UP || key == KEY_DOWN) {
				globals.dsp_stat = DSP_ON;
				dir = key == KEY_UP ? (globals.params.temperatures.high < 20 ? 1 : 0) : (globals.params.temperatures.high > 0 ? -1 : 0);
				if (dir != 0) {
					globals.params.temperatures.high += dir;
					displayTemp((int)globals.params.temperatures.high);	// DSP_ON
					TM1637_display_digit(0, _DSP_H);
				}
			} else if (key == KEY_SET) {
				globals.submode++;
			}
			break;
		case 2:
			globals.dsp_stat = DSP_BLINK;
			displayTemp((int)globals.params.temperatures.low);
			TM1637_display_digit(0, _DSP_L);
			globals.submode++;
			break;
		case 3:
			if (key == KEY_UP || key == KEY_DOWN) {
				globals.dsp_stat = DSP_ON;
				dir = key == KEY_UP ? (globals.params.temperatures.low < 20 ? 1 : 0) : (globals.params.temperatures.low > 0 ? -1 : 0);
				if (dir != 0) {
					globals.params.temperatures.low += dir;
					displayTemp((int)globals.params.temperatures.low);	// DSP_ON
					TM1637_display_digit(0, _DSP_L);
				}
			} else if (key == KEY_SET) {
				globals.submode = SUBMODE_EXIT;
			}
			break;
		case SUBMODE_EXIT:
			globals.dsp_stat = DSP_OFF;
			globals.mode = MODE_WATCH;
			globals.submode = 0;
			rc = MDS_DONE;
			break;
	}
	return rc;
}

/**
 * convert number to display / ascii representation
 *
 * - display has fixed 4 digit, value goes right
 * - ascii has up to 5 characters including decimal point (num is times 10)
 */
static char buffer[6];
uint8_t *num_2_value(int16_t num, uint8_t sign, uint8_t ascii, uint8_t dot)
{
	register char *bp = buffer;

	if (num >= 100) {	// 3 digits + sign
		if (sign) {
			*bp++ = sign;
		}
		*bp++ = (char)((num / 100) % 10) + (ascii ? '0' : 0);
		
		} else {					// 2 digits + sign
		if (!ascii) {
			*bp++ = _DSP_BLANK;
		}
		if (sign) {
			*bp++ = sign;
		}
	}
	if (num >= 10 || dot) {
		*bp++ = (char)((num / 10) % 10) + (ascii ? '0' : 0);
		if (ascii && dot) {
			*bp++ = '.';
		}
	}
	*bp = (char)(num % 10) + (ascii ? '0' : 0);
	if (ascii) {
		*++bp = 0;
	}
	return (uint8_t *)buffer;
	
}

/**
 * convert binary temperature to display / ascii representation
 *
 * - display has fixed 4 digit, temp goes right
 * - ascii has up to 5 characters including decimal point
 */
uint8_t *temp_2_value(int16_t temperature, uint8_t ascii)
{
	register uint8_t sign = 0;
	
	// calculate to [°]/10
	if (temperature < 0) {
		temperature *= -5;
		sign = ascii ? '-' : _DSP_MINUS;
	} else {
		temperature *= 5;
		sign = ascii ? 0 : _DSP_BLANK;
	}
	return num_2_value(temperature, sign, ascii, 1);
}

/**
 * show temperature (binary input value, DS18S20 resolution = 0.5[°])
 */
void displayTemp(int16_t temperature)
{
	switch (temperature) {
		case DS18x20_NO_VALUE:
			TM1637_clear();
			globals.dsp_stat = DSP_OFF;
			break;
		case DS18x20_NO_RESET:
			globals.dsp_stat = DSP_BLINK;
			TM1637_display_msg(MSG_no_r);
			break;
		case DS18x20_NO_DATA:
			globals.dsp_stat = DSP_BLINK;
			TM1637_display_msg(MSG_no_d);
			break;
		default:
			globals.dsp_stat = DSP_ON;
			TM1637_display_msg(temp_2_value(temperature, 0));
			break;
	}
}

