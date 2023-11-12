/*
 * frostguard.c
 *
 * Created: 18.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 */

// #define F_CPU 1E6	// 1,0 MHz - set in tool chain
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "tm1637.h"
#include "ds18x20.h"
#include "frostguard.h"
#include "globals.h"
#include "uart.h"

/**
 * timer interrupt service routine (100[ms])
 */
ISR(TIM0_COMPA_vect)
{
	static uint8_t ticks_counter = 0;
	static uint8_t mode_status = MDS_RUN;
	static uint8_t key_last = KEY_NONE;
	static uint8_t key_repeat = 0;
	register uint8_t key_scanned, key, current_mode;

	/*
	 * key scanning
	 * - key released -> KEY_NONE
	 * - key hit-> KEY_xxx
	 * - key long hold -> KEY_xxx - KEY_NONE - KEY_xxx_L
	 */
	#define KEY_REPEAT_MAX 4
	
	key = KEY_NONE;
	key_scanned = TM1637_keyscan();
	if (key_scanned != KEY_NONE) {
		
		if (key_last == KEY_NONE) {
			// key pressed & key was released
			key = key_scanned & 0x0F;
		} else {
			if (key_last == key_scanned) {
				if (key_repeat <= KEY_REPEAT_MAX) key_repeat++;
			} else {
				key_repeat = 0;
			}
			if (key_repeat == KEY_REPEAT_MAX) {
				key = (key_scanned << 4) | (key_scanned & 0x0F);
			} else if (key_repeat > KEY_REPEAT_MAX) {
				key = KEY_NONE;
			}
		}
	} else {
		key_repeat = 0;
	}
	key_last = key_scanned;

	/*
	 * 400[ms] blinking period and
	 * system ticks (per 1[s]) collection
	 */
	globals.blinker = (ticks_counter / 4) & 0x01;
	if (++ticks_counter == TEN_SECONDS) {
		ticks_counter = 0;
	}
	if (ticks_counter % ONE_SECOND == 0) {
		globals.params.timestamp++;
	}
	/*
	 * display and colon disable/enable/blink function
	 */
	if (globals.dsp_stat < DSP_BLINK) {
		TM1637_enable(globals.dsp_stat);
	} else {
		TM1637_enable(globals.blinker);
	}
	if (globals.col_stat < DSP_BLINK) {
		TM1637_display_colon(globals.col_stat);
	} else {
		TM1637_display_colon(globals.blinker);
	}

	/*
	 * mode dispatcher
	 */
	static uint8_t reset = 0;
	/*
	 * KEY_SET_L handling:
	 * - mode WATCH -> MENU
	 * - other -> WATCH (if not in reset)
	 */
	if (!reset && key == KEY_SET_L) {
		globals.submode = SUBMODE_EXIT;
		key = KEY_NONE;
	}

	current_mode = globals.mode;
	switch (globals.mode) {
		case MODE_RESET:
			reset = 1;
			globals.mode = MODE_TEMPS;
			break;

		case MODE_TEMPS:
			mode_status = mode_temperatures(key);
			if (mode_status == MDS_DONE && reset) {
				mode_status = MDS_RUN;
				globals.mode = MODE_DATIME;
				reset = 0;
			}
			break;

		case MODE_DATIME:
			mode_status = mode_datetime(key);
			break;

		case MODE_WATCH:
			mode_status = mode_watch(key);
			break;

		case MODE_MENU:
			mode_status = mode_menu(key);
			break;
		
		case MODE_IRRIG:
			mode_status = mode_irrigate(key);
			break;

		case MODE_BRIGHT:
			mode_status = mode_brightness(key);
			break;

		case MODE_DATA:
			mode_status = mode_data(key);
			break;
	}

	if (mode_status == MDS_DONE) {
		if (current_mode & (MODE_TEMPS | MODE_DATIME | MODE_BRIGHT)) {
			eeprom_update_block(&globals.params, &eedata.params, sizeof(params_t));
		}
		mode_status = MDS_RUN;
	}
}


/**
 * Initializations and go to sleep
 *
 * Timer:
 *
 *   OCRnx = (F_CPU / (prescaler * f[OCnx])) - 1
 *	
 *  prescaler = 1024	)
 *  F_CPU = 1MHz		)=>  OCRnx = 99
 *  f[OCnx] = 100Hz		)
 *
 */
int main(void)
{
	/*
	 * initialize globals 
	 */
	globals.mode = MODE_WATCH;
	eeprom_read_block(&globals.params, &eedata.params, sizeof(params_t));
	if (globals.params.brightness == EEUNSET) {
		globals.mode = MODE_RESET;
		globals.params.brightness = DEFAULT_BRIGHTNESS;
		globals.params.temperatures.low = BINTEMP(1.0);
		globals.params.temperatures.high = BINTEMP(3.0);
		globals.params.minmax.low = BINTEMP(60.0);
		globals.params.minmax.high = BINTEMP(-55.0);
		globals.params.timestamp = DT_2021_4_5_12_0_0;
		globals.params.write = 0;
	}

	/*
	 * initialize i/o:
	 *  0 - out TM1637 clock
	 *  1 - i/o TM1637 DIO
	 *  2 - out irrigation relay (0 = on / 1 = off)
	 *  3 - i/o DS18x20 temperature sensor / uart tx
	 *  4 - out DS18X20 conversion power (0 = on / 1 = off)
	 *  5 - reset (not used)
	 */
	TM1637_init(0, globals.params.brightness);	// disable / brightness
	TM1637_display_colon(0);
	TM1637_clear();

	globals.dsp_stat = DSP_ON;
	DS18x20_PWRINIT();
	IRRI_INIT();
	IRRI_OFF();
	/*
	 * timer init - see calculation above
	 */
	TCCR0A |= _BV(WGM01);			// CTC modus / OC0A + OC0B disconnected
	TCCR0B = _BV(CS02) | _BV(CS00);	// 1024 prescaler
	OCR0A = 99;
	TIMSK |= _BV(OCIE0A);			// enable Timer/Counter0 compare interrupt
	sei();
	/*
	 * goto sleep
	 */	
	set_sleep_mode(SLEEP_MODE_IDLE);
	sleep_enable();
	while (1)
	{
		sleep_cpu();
	}
}
