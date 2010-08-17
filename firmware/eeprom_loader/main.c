#include <avr/io.h>      // this contains all the IO port definitions
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
//#include <i2c.h>
#include <stdlib.h>
#include "../ratt.h"

extern uint8_t EE_DATA[];
extern uint8_t EE_INIT;
extern uint8_t EE_ALARM_HOUR;
extern uint8_t EE_ALARM_MIN;
extern uint8_t EE_BRIGHT;
extern uint8_t EE_REGION;
extern uint8_t EE_TIME_FORMAT;
extern uint8_t EE_SNOOZE;
extern uint8_t EE_STYLE;

extern unsigned char BigFont[];

extern unsigned char Font5x7[];

extern unsigned char FontGr[];

extern uint8_t alphatable[];
extern uint8_t numbertable[];

#ifdef OPTION_DOW_DATELONG
extern uint8_t DOWText[];
extern uint8_t MonthText[];
#endif
extern uint8_t about[];

SIGNAL(TIMER1_OVF_vect) {
  PIEZO_PORT ^= _BV(PIEZO);
}

volatile uint16_t millis = 0;
volatile uint16_t animticker, alarmticker;
SIGNAL(TIMER0_COMPA_vect) {

}

SIGNAL(TIMER1_COMPA_vect) {
  PIEZO_PORT ^= _BV(PIEZO);
}

void beep(uint16_t freq, uint8_t duration) {
  // use timer 1 for the piezo/buzzer 
  TCCR1A = 0; 
  TCCR1B =  _BV(WGM12) | _BV(CS10); // CTC with fastest timer
  TIMSK1 = _BV(TOIE1) | _BV(OCIE1A);
  OCR1A = (F_CPU / freq) / 2;
  _delay_ms(duration);
  TCCR1B = 0;
  // turn off piezo
  PIEZO_PORT &= ~_BV(PIEZO);
}

SIGNAL (TIMER2_OVF_vect) {
  wdt_reset();
}

int main(void)
{
	uint8_t inverted = 0;
  uint8_t mcustate;
  uint8_t display_date = 0;

  // check if we were reset
  mcustate = MCUSR;
  MCUSR = 0;
  
  //Just in case we were reset inside of the glcd init function
  //which would happen if the lcd is not plugged in. The end result
  //of that, is it will beep, pause, for as long as there is no lcd
  //plugged in.
  wdt_disable();
  
  // set up piezo
  PIEZO_DDR |= _BV(PIEZO);
  
  TCCR0A = _BV(WGM01);
  TCCR0B = _BV(CS01) | _BV(CS00);
  OCR0A = 125;
  TIMSK0 |= _BV(OCIE0A);

  // turn backlight on
  DDRD |= _BV(3);
  TCCR2A = _BV(COM2B1); // PWM output on pin D3
  TCCR2A |= _BV(WGM21) | _BV(WGM20); // fast PWM
  TCCR2B |= _BV(WGM22);
  OCR2A = OCR2A_VALUE;
  OCR2B = 0;
  
  DDRB |= _BV(5);
  sei();
  
  beep(4000, 100);

	uint16_t i;
	for(i=0;i<1024;i++)
	{
		eeprom_write_byte((uint8_t *)i, pgm_read_byte(&EE_DATA[i+1]));
		if((i%64)==0)
			beep(4000+i,100);
	}
	while(1)
	{
		beep(4000,100);
		_delay_ms(100);
		beep(4000,100);
		_delay_ms(100);
		beep(4000,100);
		_delay_ms(100);
		beep(4000,100);
		_delay_ms(100);
		beep(4000,100);
		_delay_ms(8000);
	}
	
}