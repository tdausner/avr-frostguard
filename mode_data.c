/*
 * mode_data.c
 *
 * Created: 21.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 *
 */ 
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include "ds18x20.h"
#include "tm1637.h"
#include "frostguard.h"
#include "globals.h"
#include "uart.h"

void perform_tx();

/**
 * data transfer mode
 * - show "no d" blinking if no data
 *   -> KEY_SET leaves
 * - show "rEt " blinking if data present
 * - KEY_UP/KEY_DOWN -> toggle display "SEnd" / "rEt  " blinking
 * - KEY_SET @"SEnd" -> start transfer
 * - KEY_SET @"rEt " -> leave
 * - show "CLr " blinking after transfer
 * - KEY_UP/KEY_DOWN -> toggle display "rEt  " / "CLr " no blinking 
 * - KEY_SET @"CLr " -> clear data, leave
 * - KEY_SET @"rEt " -> leave
 */
uint8_t	mode_data(uint8_t key)
{
	static uint8_t data_mode = 0;
	uint8_t	rc = MDS_RUN;

	switch (globals.submode) {
		case 0:
			globals.dsp_stat = DSP_BLINK;
			TM1637_display_msg(globals.params.write == 0 ? MSG_no_d : MSG_rEt);
			data_mode = 0;
			globals.submode = 1;
			break;

		case 1:
			if (globals.params.write == 0) {
				if (key == KEY_SET) {
					globals.submode = SUBMODE_EXIT;
				}
			} else {
				if (key == KEY_UP || key == KEY_DOWN) {
					globals.dsp_stat = DSP_ON;
					data_mode ^= 1;
					TM1637_display_msg(data_mode ? MSG_SEnd : MSG_rEt);
				} else if (key == KEY_SET) {
					globals.submode = data_mode ? 2 : SUBMODE_EXIT;
				}
			}
			break;

		case 2: // prepare transfer
			DS18x20_OUTPUT();
			DS18x20_HIGH();
			globals.submode = 3;
			break;

		case 3: // transfer
			perform_tx();
			data_mode = 0;
			globals.dsp_stat = DSP_BLINK;
			TM1637_display_msg(MSG_rEt);
			globals.submode = 4;
			break;

		case 4:
			if (key == KEY_UP || key == KEY_DOWN) {
				globals.dsp_stat = DSP_ON;
				data_mode ^= 1;
				TM1637_display_msg(data_mode ? MSG_CLr: MSG_rEt);
			} else if (key == KEY_SET) {
				globals.submode = data_mode ? 5 : SUBMODE_EXIT;
			}
			break;

		case 5:
			globals.params.write = 0;
			globals.params.minmax.low = BINTEMP(60.0);
			globals.params.minmax.high = BINTEMP(-55.0);
			eeprom_update_block(&globals.params, &eedata.params, sizeof(params_t));
			// no break;
		case SUBMODE_EXIT:
			globals.mode = MODE_WATCH;
			globals.submode = 0;
			rc = MDS_DONE;
			break;
	}
	return rc;
}

/**
 * transmit key / value pair
 */
void uart_tx_value(char *key, char *value)
{
	uart_tx_string("  \"");
	uart_tx_string(key);
	uart_tx_string("\": ");
	uart_tx_string(value);
	uart_tx_string(",\n");
}

/**
 * perform transfer (JSON format)
 *
 * {
 *   "tH": 5.5,						temperature threshold high
 *   "tL": 2.0,						temperature threshold low
 *   "mH": 2.0,						temperature max
 *   "mL": 2.0,						temperature min
 *   "ev": [{						events
	   "n": 1,						  event number
 *     "ts": "2021-03-27 12:42",	  timestamp
 *     "tm": 1.5,					  temperature
 *     "im": 1						  irrigation mode
 *   },{
 *     ...
 *   }]
 * }
 */
void perform_tx()
{
	register uint8_t read = 0;
	event_t ev;

	DS18x20_PWROFF();
	DS18x20_OUTPUT();
	uart_tx_string("\n{\n");
	uart_tx_value("tH", (char *)temp_2_value(globals.params.temperatures.high, 1));
	uart_tx_value("tL", (char *)temp_2_value(globals.params.temperatures.low, 1));
	uart_tx_value("mH", (char *)temp_2_value(globals.params.minmax.high, 1));
	uart_tx_value("mL", (char *)temp_2_value(globals.params.minmax.low, 1));
	uart_tx_string("  \"ev\": [{");
	while (1) {
		eeprom_read_block(&ev, &eedata.events[read], sizeof(event_t));
		uart_tx_string("\n  ");
		uart_tx_value("n", (char *)num_2_value(read, 0, 1, 0));
		uart_tx_string("  ");
		uart_tx_value("ts", timestamp_2_string(ev.timestamp));
		uart_tx_string("  ");
		uart_tx_value("tm", (char *)temp_2_value(ev.temp, 1));
		uart_tx_string("    \"im\": ");
		uart_tx(ev.irri_mode + '0');
		uart_tx_string("\n  }");
		if (++read < globals.params.write) {
			uart_tx_string(",{");
		} else {
			break;
		}
	}
	uart_tx_string("]\n}\n");
	DS18x20_INPUT();
}

/**
 * store event
 */
void store_event(int16_t temp, uint8_t irri_mode)
{
	event_t event;
	register uint8_t must_write = 0;
	register uint8_t wr_params = 0;
	
	if (temp < globals.params.minmax.low) {
		globals.params.minmax.low = temp;
		wr_params = 1;
	} else if (temp > globals.params.minmax.high) {
		globals.params.minmax.high = temp;
		wr_params = 1;
	}
	if (globals.params.write < MAX_EVENTS) {
		if (globals.params.write > 0) {
			eeprom_read_block(&event, &eedata.events[globals.params.write - 1], sizeof(event_t));
			must_write = (temp < event.temp && irri_mode > 0) || (event.irri_mode != irri_mode && event.irri_mode > 0);
		} else if (temp <= globals.params.temperatures.low || irri_mode != 0) {
			must_write = 1;
		}
		if (must_write) {
			event.temp = temp;
			event.irri_mode = irri_mode;
			event.timestamp = globals.params.timestamp;
			eeprom_update_block(&event, &eedata.events[globals.params.write], sizeof(event_t));
			globals.params.write++;
			wr_params = 1;
		}
	}
	if (wr_params) {
		eeprom_update_block(&globals.params, &eedata.params, sizeof(params_t));
	}
}