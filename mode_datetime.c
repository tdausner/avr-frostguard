/*
 * mode_datim.c
 *
 * Created: 20.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 */
#include <stdint.h>
#include <avr/io.h>
#include "tm1637.h"
#include "frostguard.h"
#include "globals.h"

/**
 * set date and time mode
 *
 * - KEY_SET -> show year + 1970 blinking no colon
 * - KEY_UP/KEY_DOWN -> incr/decr year no blink
 * - KEY_SET -> show month blinking
 * - KEY_UP/KEY_DOWN -> incr/decr month no blink
 * - KEY_SET -> show day blinking
 * - KEY_UP/KEY_DOWN -> incr/decr day no blink
 * - KEY_SET -> show hour blinking + colon blinking
 * - KEY_UP/KEY_DOWN -> incr/decr hour no blink, colon blinking
 * - KEY_SET -> show min blinking + colon blinking
 * - KEY_UP/KEY_DOWN -> incr/decr min no blink, colon blinking
 * - KEY_SET -> store date + time and leave
 */
static uint8_t days_per_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static uint8_t year, month, day, hour, min, sec;

static void showDateTime(uint8_t first, uint8_t second);
static void ticks_to_datetime(register uint32_t t);
static void datetime_to_ticks();
	
uint8_t mode_datetime(uint8_t key)
{
	register uint8_t	rc = MDS_RUN;
	register uint8_t	dir;
	register uint16_t	yy;
	register uint8_t	month_days;

	switch (globals.submode) {
		case 0:
			ticks_to_datetime(globals.params.timestamp);
			yy = year + 1970;
			globals.dsp_stat = DSP_BLINK;
			showDateTime((uint8_t)(yy / 100), (uint8_t)(yy % 100));
			globals.submode++;
			break;
		case 1:
			if (key == KEY_UP || key == KEY_DOWN) {
				yy = (uint16_t)(year + (key == KEY_UP ? 1 : (year > (2021 - 1970) ? -1 : 0))) + 1970;
				globals.dsp_stat = DSP_ON;
				showDateTime((uint8_t)(yy / 100), (uint8_t)(yy % 100));
				year = (uint8_t)(yy - 1970);
			} else if (key == KEY_SET) {
				globals.dsp_stat = DSP_BLINK;
				showDateTime(month, -1);
				globals.submode++;
			}
			break;
		case 2:
			if (key == KEY_UP || key == KEY_DOWN) {
				globals.dsp_stat = DSP_ON;
				dir = key == KEY_UP ? (month < 12 ? 1 : -11) : (month > 1 ? -1 : 11);
				if (dir != 0) {
					month += dir;
					showDateTime(month, -1);
				}
			} else if (key == KEY_SET) {
				globals.dsp_stat = DSP_BLINK;
				month_days = days_per_month[month];
				if (month == 2 && (year - 2) % 4 == 0) {
					month_days = 29;
				}
				if (day > month_days) {
					day = month_days;
				}
				showDateTime(-1, day);
				globals.submode++;
			}
			break;
		case 3:
			if (key == KEY_UP || key == KEY_DOWN) {
				register uint8_t month_days = days_per_month[month];

				globals.dsp_stat = DSP_ON;
				if (month == 2 && ((year - 2) % 4) == 0) {
					month_days++;
				}
				dir = key == KEY_UP ? (day < month_days ? 1 : -(month_days - 1)) : (day > 1 ? -1 : (month_days - 1));
				if (dir != 0) {
					day += dir;
					showDateTime(-1, day);
				}
			} else if (key == KEY_SET) {
				globals.dsp_stat = globals.col_stat = DSP_BLINK;
				showDateTime(hour, -1);
				globals.submode++;
			}
			break;
		case 4:
			if (key == KEY_UP || key == KEY_DOWN) {
				globals.dsp_stat = DSP_ON;
				dir = key == KEY_UP ? (hour < 23 ? 1 : -23) : (hour > 0 ? -1 : 23);
				if (dir != 0) {
					hour += dir;
					showDateTime(hour, -1);
				}
			} else if (key == KEY_SET) {
				globals.dsp_stat = DSP_BLINK;
				showDateTime(-1, min);
				globals.submode++;
			}
			break;
		case 5:
			if (key == KEY_UP || key == KEY_DOWN) {
				globals.dsp_stat = DSP_ON;
				dir = key == KEY_UP ? (min < 59 ? 1 : -59) : (min > 0 ? -1 : 59);
				if (dir != 0) {
					min += dir;
					showDateTime(-1, min);
				}
			} else if (key == KEY_SET) {
				globals.submode = SUBMODE_EXIT;
			}
			break;
		case SUBMODE_EXIT:
			globals.col_stat = globals.dsp_stat = DSP_OFF;
			sec = 0;
			datetime_to_ticks();
			globals.mode = MODE_WATCH;
			globals.submode = 0;
			rc = MDS_DONE;
			break;
	}
	return rc;
}

/**
 * show date or time (4 digits: first two = first / last two = second)
 */
static void showDateTime(uint8_t first, uint8_t second)
{
	register uint8_t d1 = _DSP_BLANK;
	register uint8_t d2 = _DSP_BLANK;
	register uint8_t d3 = _DSP_BLANK;
	register uint8_t d4 = _DSP_BLANK;
	
	if ((signed char)first >= 0) {
		d1 = first / 10;
		d2 = first % 10;
	}
	if ((signed char)second >= 0) {
		d3 = second / 10;
		d4 = second % 10;
	}
	TM1637_display_digit(0, d1);
	TM1637_display_digit(1, d2);
	TM1637_display_digit(2, d3);
	TM1637_display_digit(3, d4);
}

/**
 * convert ticks to year, month, day, hour, min, sec
 */
static void ticks_to_datetime(register uint32_t t)
{
	register uint8_t j, m;

	sec = (uint8_t)(t % 60);
	t /= 60;
	min = (uint8_t)(t % 60);
	t /= 60;
	hour = (uint8_t)(t % 24);
	t /= 24;
	year = (uint8_t)(t / 365);
	t -= (uint32_t)year * 365 + ((year - 2) / 4); // (year entered - 1968) / 4
	month = 1;
	for (m = 1; m < sizeof(days_per_month); m++) {
		j = days_per_month[m];
		if (m == 2 && ((year - 2) % 4) == 0) {
			j++;
		}
		if (t > j) {
			month++;
			t -= j;
		} else {
			day = t + 1;
			break;
		}
	}
}

/**
 * convert timestamp to ascii timestamp
 */
static char buffer[22];
char *timestamp_2_string(uint32_t ts)
{
	register uint16_t yy;

	ticks_to_datetime(ts);
	yy = year + 1970;
	buffer[0] = '"';
	buffer[1] = (yy / 1000 % 10) + '0';
	buffer[2] = (yy / 100 % 10) + '0';
	buffer[3] = (yy / 10 % 10) + '0';
	buffer[4] = (yy  % 10) + '0';
	buffer[5] = '-';
	buffer[6] = (month / 10 % 10) + '0';
	buffer[7] = (month  % 10) + '0';
	buffer[8] = '-';
	buffer[9] = (day / 10 % 10) + '0';
	buffer[10] = (day  % 10) + '0';
	buffer[11] = ' ';
	buffer[12] = (hour / 10 % 10) + '0';
	buffer[13] = (hour  % 10) + '0';
	buffer[14] = ':';
	buffer[15] = (min / 10 % 10) + '0';
	buffer[16] = (min  % 10) + '0';
	buffer[17] = ':';
	buffer[18] = (sec / 10 % 10) + '0';
	buffer[19] = (sec  % 10) + '0';
	buffer[20] = '"';
	buffer[21] = 0;
	return buffer;
}

/**
 * convert year, month, day, hour, min, sec to ticks 
 */
static void datetime_to_ticks()
{
	register uint16_t days;
	register uint8_t m;

	days = (uint16_t)day - 1;	// offset 0	
	for (m = 1; m < sizeof(days_per_month) && m < month; m++) {
		days += (uint16_t)days_per_month[m];
	}
	globals.params.timestamp = ((((((uint32_t)year * 365) + (year - 2) / 4 + (uint32_t)days) * 24) + (uint32_t)hour) * 60 + (uint32_t)min) * 60 + (uint32_t)sec;
}