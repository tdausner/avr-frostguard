/*
 * mode_brightness.c
 *
 * Created: 21.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 *
 */ 
#include <stdint.h>
#include "tm1637.h"
#include "frostguard.h"
#include "globals.h"

/**
 * brightness mode
 *
 * - "bri " is displayed, blink on
 * - KEY_UP/KEY_DOWN -> incr/decr brightness
 * - KEY_SET -> set brightness into globals.params.brightness, leave
 * - KEY-SET_L -> save brightness -> MODE_WATCH, leave
 * 
 */
uint8_t	mode_brightness(uint8_t key)
{
	register uint8_t rc = MDS_RUN;
	register uint8_t bri = globals.params.brightness;

	globals.dsp_stat = DSP_BLINK;
	if (key == KEY_UP ||key == KEY_DOWN) {
		bri += key == KEY_UP ? (bri == MAX_BRIGHTNESS ? -MAX_BRIGHTNESS : 1 ) : (bri == 0 ? MAX_BRIGHTNESS : -1);
		globals.params.brightness = bri;
		TM1637_set_brightness(bri);
	} else if (key == KEY_SET) {
		globals.dsp_stat = DSP_ON;
		globals.mode = MODE_WATCH;
		globals.submode = 0;
		rc = MDS_DONE;
	}
	TM1637_display_digit(3, bri);
	return rc;
}
