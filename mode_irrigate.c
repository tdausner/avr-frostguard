/*
 * mode_irrigate.c
 *
 * Created: 21.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 *
 */ 
#include <stdint.h>
#include <avr/io.h>
#include "tm1637.h"
#include "frostguard.h"
#include "globals.h"

/**
 * irrigate mode
 *
 * - show "oFF " not blinking
 * - KEY_UP/KEY_DOWN -> toggle display "on  " / "oFF "
 *   - @"oFF " -> IRRI_OFF(), blink off
 *   - @"on  " -> IRRI_ON(), blink on
 * - KEY-SET_L -> set MODE_WATCH into globals.mode, leave
 * 
 */
uint8_t	mode_irrigate(uint8_t key)
{
	uint8_t	rc = MDS_RUN;
	static uint8_t irri_mode = 0;
	register uint8_t *msg;

	switch (globals.submode) {
		case 0:
			globals.dsp_stat = DSP_ON;
			TM1637_display_msg(MSG_oFF);
			irri_mode = 0;
			globals.submode++;
			break;
		case 1:
			if (key == KEY_UP || key == KEY_DOWN) {
				irri_mode ^= 1;
				if (irri_mode) {
					globals.dsp_stat = DSP_BLINK;
					IRRI_ON();
					msg = MSG_on;
				}  else {
					globals.dsp_stat = DSP_ON;
					IRRI_OFF();
					msg = MSG_oFF;
				}
				TM1637_display_msg((const uint8_t *)msg);
			}
			break;
		case SUBMODE_EXIT:
			globals.mode = MODE_WATCH;
			globals.submode = 0;
			IRRI_OFF();
			rc = MDS_DONE;
			break;
	}
	return rc;
}
