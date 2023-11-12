/*
 * globals.c
 *
 * Created: 18.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 */ 

#include <avr/io.h>
#include "frostguard.h"
#include "tm1637.h"
#include "globals.h"

/**
 * globals
 */
globals_t globals = {
	.mode = MODE_RESET,
	.submode = 0,	// each mode function must set to 0 on leave
	.blinker = 0,
	.col_stat = 0,	// colon off
	.dsp_stat = 0	// display off
};

eedata_t EEMEM eedata = {
	.params = {
		.brightness = 0xFF,
		.write = 0,
		.timestamp = DT_2021_4_5_12_0_0,
		.temperatures = { 
			.high = 0xFF, 
			.low = 0xFF 
		},
		.minmax = {
			.high = 0xFF,
			.low = 0xFF
		}
	}
};

/**
 * messages for menus et.al.
 */
const uint8_t messages[] = {
// menu messages - keep at begin of array and in order (see mode_menu.c)
	0x0D,		0x0A,		_DSP_t,		0x0A,		// dAtA
	_DSP_i,		_DSP_r,		_DSP_r,		_DSP_i,		// irri
	0x0B,		_DSP_r,		_DSP_i,		_DSP_BLANK,	// bri_
	_DSP_t,		0x0E,		_DSP_n,		_DSP_P,		// tEnP
	0x0D,		0x0A,		_DSP_t,		0x0E,		// dAtE
// end of menu messages	
	0x05,		0x0E,		_DSP_n,		0x0d,		// SEnd
	_DSP_o,		_DSP_n,		_DSP_BLANK,	_DSP_BLANK,	// on__
	_DSP_o,		0x0F,		0x0F,		_DSP_BLANK,	// oFF_
	0x0C,		_DSP_L,		_DSP_r,		_DSP_BLANK,	// CLr_
	_DSP_r,		0x0E,		_DSP_t,		_DSP_BLANK,	// rEt_
	_DSP_n,		_DSP_o,		_DSP_BLANK,	0x0D,		// no_d
	_DSP_n,		_DSP_o,		_DSP_BLANK,	_DSP_r		// no_r
};
