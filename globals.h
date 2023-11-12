/*
 * globals.h
 *
 * Created: 18.03.2021
 *
 * (c) TDSystem Thomas Dausner 2021
 */ 
#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <avr/eeprom.h>

/**
 * data types
 */
typedef struct
{
	int8_t	low;
	int8_t	high;
	
} temperatures_t;

typedef struct params		// runtime parameters / copy in eeprom
{
	temperatures_t	temperatures;	// threshold temperatures
	temperatures_t	minmax;			// min/max temperatures
	uint32_t		timestamp;		// reference January 1st, 1970, 00:00:00 in [s]
	uint8_t			brightness;
	int8_t			write;			// event data eeprom write index
		
} params_t;

#define DT_2021_4_5_12_0_0 ((((uint32_t)(2021 - 1970) * 365 + (uint32_t)((2021 - 1968) / 4) + (31 + 28 + 31 + 3)) * 24 + 12) * 60 * 60) // 01.01.2021 12:00:00

typedef struct		// irrigation event data
{
	uint32_t	timestamp;	// 1[s] resolution timestamp since 1970-01-01 00:00:00
	int8_t		temp;		// binary temperature 0.5[°] resolution
	uint8_t		irri_mode;	// irrigation mode
	
} event_t;

/**
 * globals
 */
typedef struct
{
	params_t	params;
	uint8_t		mode;
	uint8_t		submode;
	uint8_t		blinker;
	uint8_t		col_stat;	// colon status off/on/blinking
	uint8_t		dsp_stat;	// display status off/on/blinking
	
} globals_t;

extern globals_t globals;

/**
 * display messages for menus et.al.
 */
extern const uint8_t messages[];
// menu messages definitions - keep at begin and in order (see mode_menu.c)
#define MSG_dAtA	((uint8_t *)(messages +  0))
#define MSG_irri	((uint8_t *)(messages +  4))
#define MSG_bri		((uint8_t *)(messages +  8))
#define MSG_tEnP	((uint8_t *)(messages + 12))
#define MSG_dAtE	((uint8_t *)(messages + 16))
// end of menu messages - other messages
#define MSG_SEnd	((uint8_t *)(messages + 20))
#define MSG_on		((uint8_t *)(messages + 24))
#define MSG_oFF		((uint8_t *)(messages + 28))
#define MSG_CLr		((uint8_t *)(messages + 32))
#define MSG_rEt		((uint8_t *)(messages + 36))
#define MSG_no_d	((uint8_t *)(messages + 40))
#define MSG_no_r	((uint8_t *)(messages + 44))

/**
 * eeprom data
 */
#define EEPROM_SIZE	(E2END + 1)
#define MAX_EVENTS	((EEPROM_SIZE - sizeof(params_t) - sizeof(int8_t)) / sizeof(event_t))

#define EEUNSET	0xFF	// eeprom data unset

typedef struct
{
	params_t	params;
	event_t		events[MAX_EVENTS];

} eedata_t;

extern eedata_t EEMEM eedata;

#endif /* GLOBALS_H_ */