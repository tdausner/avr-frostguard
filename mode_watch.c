/*
 * mode_watch.c
 *
 * Created: 20.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 *
 */ 
#include <stdint.h>
#include <avr/io.h>
#include "ds18x20.h"
#include "tm1637.h"
#include "frostguard.h"
#include "globals.h"

/**
 * watch temperature mode
 *
 * - display off
 * - start measurement, colon on
 * - after CONVERSION_TIME
 *   - colon off
 *   - get temperature
 * - KEY_SET -> show temperature 10[s]
 * - KEY-SET_L -> (global.submode = SUBMODE_EXIT in frostguard.c) -> MODE_MENU
 * 
 */
static uint8_t measure_count;
static uint8_t display_count;
static int16_t temp = DS18x20_NO_VALUE;
static uint8_t irri_mode = 0;
static uint8_t pulse_timer = 0;
static uint16_t irri_timer = 0;

uint8_t	mode_watch(uint8_t key)
{
	uint8_t	rc = MDS_RUN;

	if (globals.submode == 0) {
		/*
		 * init MODE_WATCH 
		 */
		measure_count = 0;
		display_count = 1;
		globals.dsp_stat = DSP_ON;
		globals.col_stat = DSP_OFF;
		TM1637_clear();
		globals.submode = 1;
		
	} else if (globals.submode == SUBMODE_EXIT) {
		/*
		 * end MODE_WATCH -> MODE_MENU
		 */
		globals.mode = MODE_MENU;
		globals.submode = 0;
		globals.col_stat = DSP_OFF;
		irri_mode = 0;
		IRRI_OFF();
		rc = MDS_DONE;
		
	} else { // globals.submode == 1
		if (key == KEY_SET) {
			display_count = 1;
		}
		/*
		 * temperature measurement
		 */
		switch (measure_count) {
			case 0:
				globals.col_stat = DSP_ON;
				DS18x20_PWRINIT();
				DS18x20_PWRON();	// give sensor 200[ms] power
				measure_count++;
				break;
				
			case 2:
				DS18x20_PWROFF();
				temp = DS18x20_startcv();
				measure_count = temp == DS18x20_NO_RESET ? 0 : (measure_count + 1);
				break;

			case CONVERSION_TIME + 2:
				temp = DS18x20_readtemp();
				if (temp == DS18x20_NO_DATA || temp < BINTEMP(-20.0) || temp >= BINTEMP(40.0)) {
					measure_count = 0;
				} else {
					/*
					 * calculate irrigation mode from temperature
					 */
					if ((int8_t)temp > globals.params.temperatures.high) {
						// stop irrigation & pulse timer
						irri_mode = pulse_timer = 0;
					} else if ((int8_t)temp <= globals.params.temperatures.low) {
						// start irrigation & pulse timer
						irri_mode = pulse_timer = 1;
					} else if (irri_mode >= 1) {
						irri_mode = (int8_t)temp - globals.params.temperatures.low + 1;
					}
					store_event(temp, irri_mode);
					globals.col_stat = DSP_OFF;
					measure_count++;
				}
				break;

			case TEN_SECONDS:
				measure_count = 0;
				break;

			default:
				measure_count++;
				break;
		}
		/*
		 * show temp (or error) if applicable
		 */
		if (temp > DS18x20_NO_VALUE) {	// error
			globals.col_stat = DSP_OFF;
			display_count = 1;
		} else {
			/*
			 *  pulse irrigation from irri_mode
			 *
			 *   0: off
			 *   1: constant on
			 *   2: 60[s] on /     30[s] off
			 *   3: 60[s] on / 2 x 30[s] off
			 *   4: 60[s] on / 3 x 30[s] off
			 *   ...
			 */
			if (pulse_timer == 0) {
				// stop pulse timer
				IRRI_OFF();
				irri_timer = 0;
			} else if (pulse_timer == 1) {
				// on phase
				if (irri_timer == 0) {
					IRRI_ON();
					irri_timer = SIXTY_SECONDS;
				}
				pulse_timer = irri_mode;
			} else {
				// off phase
				if (irri_timer == 0) {
					IRRI_OFF();
					irri_timer = THIRTY_SECONDS;
				}
			}
			if (irri_timer > 0) {
				if (--irri_timer == 0) {
					if (pulse_timer == 1) {
						pulse_timer = irri_mode;
					} else {
						pulse_timer--;
					}
				}
			}
		}
		if (display_count > TEN_SECONDS) {
			display_count = 0;
			TM1637_clear();
		} else if (display_count > 0) {
			displayTemp(temp);
			display_count++;
		}
	}
	return rc;
}
