/*
 * mode_menu.c
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
 * menu mode
 *
 * - KEY_UP/KEY_DOWN -> incr/decr menu item
 * - KEY_SET -> set selected menu item into globals.mode, leave
 * - KEY-SET_L -> set MODE_WATCH into globals.mode, leave
 * 
 * const uint8_t messages[] in globals.c holds the 4 digit messages for the menu entries
 *
 * resulting message pointers are defined in globals.h
 *
 * keep order in array next[] as index in next[] is proportional to index in messages[]
 */
static const uint8_t next[] = { MODE_DATA, MODE_IRRIG, MODE_BRIGHT, MODE_TEMPS, MODE_DATIME };
#define MAX_NEXT (sizeof(next) - 1)

uint8_t	mode_menu(uint8_t key)
{
	uint8_t	rc = MDS_RUN;
	register uint8_t sm = globals.submode;

	if (globals.submode == SUBMODE_EXIT) {
		globals.mode = MODE_WATCH;
		globals.submode = 0;
		globals.dsp_stat = DSP_OFF;
		rc = MDS_DONE;
	} else {
		globals.dsp_stat = DSP_ON;
		TM1637_display_msg(messages + 4 * sm);

		if (key == KEY_UP ||key == KEY_DOWN) {
			sm += key == KEY_UP ? (sm == MAX_NEXT ? -MAX_NEXT : 1 ) : (sm == 0 ? MAX_NEXT : -1);
		} else if (key == KEY_SET) {
			globals.mode = next[globals.submode];
			sm = 0;
			rc = MDS_DONE;
		}
		globals.submode = sm;
	}
	return rc;
}
