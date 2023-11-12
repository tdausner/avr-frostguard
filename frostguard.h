/*
 * frostguard.h
 *
 * Created: 18.03.202
 *
 * (c) TDSystem Thomas Dausner 2021
 */ 

#ifndef FROSTGUARD_H_
#define FROSTGUARD_H_

/**
 * timer ISR(TIM0_COMPA_vect)@100[ms] related stuff
 */
#define CONVERSION_TIME (((DS18x20_CVT / 100) * 100) < DS18x20_CVT ? (DS18x20_CVT / 100) + 1 : (DS18x20_CVT / 100))

#define ONE_SECOND		10
#define TEN_SECONDS		100
#define THIRTY_SECONDS	300
#define SIXTY_SECONDS	600

/**
 * modes of the state machine
 */
#define MODE_UNSET	0		// no mode set
#define MODE_RESET	_BV(0)	// power on
#define MODE_TEMPS	_BV(1)	// set temperatures
#define MODE_DATIME	_BV(2)	// set date/time
#define MODE_WATCH	_BV(3)	// watch temperature
#define MODE_MENU	_BV(4)	// menu
#define MODE_IRRIG	_BV(5)	// manual irrigation
#define MODE_BRIGHT	_BV(6)	// set brightness
#define MODE_DATA	_BV(7)	// retrieve irrigation data

#define SUBMODE_EXIT	99	// submode: exit mode
/*
 * mode function status return codes
 */
#define MDS_RUN		0
#define MDS_DONE	1

/**
 * key codes from TM1637_keyscan()
 */
#define KEY_NONE	0xFF	// all keys released
#define KEY_UP		0x07	// key UP hit
#define KEY_DOWN	0x06	// key DOWN hit
#define KEY_SET		0x05	// key SET hit
#define KEY_UP_L	0x77	// key UP long hold
#define KEY_DOWN_L	0x66	// key DOWN long hold
#define KEY_SET_L	0x55	// key SET long hold

/**
 * display status
 */
#define DSP_OFF		0
#define DSP_ON		1
#define DSP_BLINK	2

/**
 * port B pin 2 used for irrigation relay control (low = on)
 */
#define IRRI_INIT()		(DDRB |= _BV(DDB2))
#define	IRRI_ON()		(PORTB &= ~_BV(PB2))
#define	IRRI_OFF()		(PORTB |= _BV(PB2))

/**
 * brightness (0...TM1637_BRIGHTNESS_MAX)
 */
#define DEFAULT_BRIGHTNESS 5
#define MAX_BRIGHTNESS 7

/**
 * binary temperature (0.5[°] resolution)
 */
#define BINTEMP(x)	(x * 2)

/**
 * mode and mode related functions
 *
 * all mode functions return 1 for mode done / 0 for mode not done
 */
uint8_t	mode_temperatures(uint8_t key);	// mode_temp.c - set temperatures
void	displayTemp(int16_t temperature);
uint8_t *num_2_value(int16_t num, uint8_t sign, uint8_t ascii, uint8_t dot);
uint8_t *temp_2_value(int16_t temperature, uint8_t ascii);
uint8_t	mode_datetime(uint8_t key);		// mode_datetime.c - set date and time
void update_datetime();
char *timestamp_2_string(uint32_t ts);
uint8_t	mode_watch(uint8_t key);		// mode_watch.c - watch / show temperature 
uint8_t	mode_menu(uint8_t key);			// mode_menu.c - menu selection
uint8_t	mode_brightness(uint8_t key);	// mode_brightness.c
uint8_t	mode_irrigate(uint8_t key);		// mode_irrigate.c
uint8_t	mode_data(uint8_t key);			// mode_data.c - transfer data
void store_event(int16_t temp, uint8_t irri_mode);

#endif /* FROSTGUARD_H_ */