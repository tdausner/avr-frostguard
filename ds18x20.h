/*
 * DS18x20 lib
 *
 * based on ds18b20 lib 0x02 (copyright (c) Davide Gironi, 2012)
 *
 * mods (c) TDSystem Thomas Dausner 2021
 *
 * Released under GPLv3.
 * Please refer to LICENSE file for licensing information.
 *
 * References:
 * + Using DS18x20 digital temperature sensor on AVR micro controllers
 *   by Gerard Marull Paretas, 2007
 *   https://teslabs.com/openplayer/docs/docs/other/ds18b20_pre1.pdf
 *
 * + APPLICATION NOTE 4377
 *   1-WIRE PROTOCOL PDF OF DS18S20 VS DS18B20 DIGITAL THERMOMETERS
 *   maxim integrated
 *   https://www.maximintegrated.com/en/design/technical-documents/app-notes/4/4377.html
 *
 *
 * This library supports two types of sensors (and compatible types)
 * supporting different temperature resolutions:
 *
 * - DS18B20 resolution 9, 10, 11 or 12 bit
 * - DS18S20 resolution 9 bit
 *
 * Depending on the sensor the conversion times differ (rounded up to [ms]):
 *
 * - DS18B20 conversion times
 *   +  94[ms] @resolution  9 bit (0.5°C)
 *   + 188[ms] @resolution 10 bit (0.25°C)
 *   + 175[ms] @resolution 11 bit (0.125°C)
 *   + 750[ms] @resolution 12 bit (0.0625°C)
 * - DS18S20 conversion time constant 750[ms] @resolution 9 bit (0.5°C)
 *
 * Both sensor can be operated with three wires or in a two wires 
 * parasite-powered configuration:
 *
 * - three wires: DQ, Vdd and GND -> the AVR needs one port pin for DQ
 * - two wires: DQ and GND -> the AVR needs one port pin for DQ and
 *   another port pin for pullup control (see "Powering The DS18S20"
 *   section in https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf)
 *   The library supports DQ and pullup control residing on the same port.
 *
 * This library support asynchronous or synchronous temperature conversion:
 *
 * - sync:  call to DS18x20_gettemp() returns after conversion finished
 * - async: call to DS18x20_start() returns after start of conversion
 *          -> conversion time delay must be granted in calling software
 *          call to DS18x20_read() returns after read of temperature
 */
#ifndef DS18x20_H_
#define DS18x20_H_

#include <avr/io.h>

/* ----------------- sensor definition section -----------------
 *
 * Type of sensor - set to DS18B10 or DS18S10
 */
#define DS18x20_TYPE	DS18S10
/*
 * Sensor resolution - set to 9, 10, 11 or 12 for DS18B20
 *                     (no effect for DS18S20)
 */
#define DS18x20_RESOL	9
/*
 * Operation mode - set to
 * - 0 for no parasite-powered operation
 * - 1 for parasite-power on output high
 * - 2 for parasite-power on output low
 */ 
#define DS18x20_PARAPWR	2
/*
 * connection setup 
 */
#define DS18x20_PORT	PORTB
#define DS18x20_DDR		DDRB
#define DS18x20_PIN		PINB
#define DS18x20_PWR		PB4
#define DS18x20_DQ		PB3
/*
 * end of sensor configuration section
 * -----------------------------------
 *
 *
 * functions
 */
int16_t DS18x20_gettemp();	// for sync operation
int16_t DS18x20_startcv();	// for async operation
int16_t DS18x20_readtemp();	// for async operation

/*
 * sensor macros
 */
#if DS18x20_PARAPWR == 0
#  define	DS18x20_PWRINIT()
#  define	DS18x20_PWRON()
#  define	DS18x20_PWROFF()  
#else
#  if DS18x20_PARAPWR == 1
#    define	DS18x20_PWRON()		(DS18x20_PORT |= _BV(DS18x20_PWR))
#    define	DS18x20_PWROFF()	(DS18x20_PORT &= ~_BV(DS18x20_PWR))
#  endif
#  if DS18x20_PARAPWR == 2
#    define	DS18x20_PWRON()		(DS18x20_PORT &= ~_BV(DS18x20_PWR))
#    define	DS18x20_PWROFF()	(DS18x20_PORT |= _BV(DS18x20_PWR))
#  endif
#  define	DS18x20_PWRINIT()	((DS18x20_DDR |= _BV(DS18x20_PWR)) && DS18x20_PWROFF())
#endif

#if DS18x20_TYPE == DS18S10
#  define DS18x20_MAX	0x0AA	// max value on 9 Bit resolution: 85.0°C
#  define DS18x20_RES	0
#  define DS18x20_CVT	750
#else
#  if DS18x20_RESOL	== 9
#    define DS18x20_MAX	0x00FA	// max value on 9 Bit resolution: 125°C
#    define DS18x20_RES	0x1F
#    define DS18x20_CVT	 94
#  endif
#  if DS18x20_RESOL	== 10
#    define DS18x20_MAX	0x01F4	// max value on 10 Bit resolution: 125°C
#    define DS18x20_RES	0x3F
#    define DS18x20_CVT	188
#  endif
#  if DS18x20_RESOL	== 11
#    define DS18x20_MAX	0x03E8	// max value on 11 Bit resolution: 125°C
#    define DS18x20_RES	0x5F
#    define DS18x20_CVT	375
#  endif
#  if DS18x20_RESOL	== 12
#    define DS18x20_MAX	0x07D0	// max value on 12 Bit resolution: 125°C
#    define DS18x20_RES	0x6F
#    define DS18x20_CVT	750
#  endif
#endif // DS18x20__TYPE == DS18S10

/*
 * return values (int16_t)
 *
 * 0x07D0...0xFC90  ->  +125[°]...-55[°] on 12 Bit resolution
 * 0x00AA...0xFF92  ->   +85[°]...-55[°] on  9 Bit resolution
 */
#define DS18x20_NO_VALUE	(DS18x20_MAX + 1)	// ok - no value sampled
#define DS18x20_NO_RESET	(DS18x20_MAX + 2)	// error - no sensor reset
#define DS18x20_NO_DATA		(DS18x20_MAX + 3)	// error - no sensor data


/*
 * commands
 */
#define DS18x20_CMD_CONVERTTEMP		0x44
#define DS18x20_CMD_RSCRATCHPAD		0xbe
#define DS18x20_CMD_WSCRATCHPAD		0x4e
#define DS18x20_CMD_CPYSCRATCHPAD	0x48
#define DS18x20_CMD_RECEEPROM		0xb8
#define DS18x20_CMD_RPWRSUPPLY		0xb4
#define DS18x20_CMD_SEARCHROM		0xf0
#define DS18x20_CMD_READROM			0x33
#define DS18x20_CMD_MATCHROM		0x55
#define DS18x20_CMD_SKIPROM			0xcc
#define DS18x20_CMD_ALARMSEARCH		0xec

#define	DS18x20_HIGH()		(DS18x20_PORT |= _BV(DS18x20_DQ))
#define	DS18x20_LOW()		(DS18x20_PORT &= ~_BV(DS18x20_DQ))
#define	DS18x20_OUTPUT()	(DS18x20_DDR |= _BV(DS18x20_DQ))
#define	DS18x20_INPUT()		(DS18x20_DDR &= ~_BV(DS18x20_DQ))
#define	DS18x20_READ() 		(((DS18x20_PIN & _BV(DS18x20_DQ)) > 0) ? 1 : 0)


#endif
