#include <avr/io.h>      // this contains all the IO port definitions
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
#include <i2c.h>
#include <stdlib.h>
#include "util.h"
#include "ratt.h"
#include "ks0108.h"
#include "glcd.h"
#include "dispatch.h"


volatile uint8_t time_s, time_m, time_h;
volatile uint8_t old_h, old_m, old_s;
volatile uint8_t timeunknown = 1;
volatile uint8_t date_m, date_d, date_y;
volatile uint8_t alarming, alarm_on, alarm_tripped, alarm_h, alarm_m;
volatile uint8_t displaymode;
volatile uint8_t volume;
volatile uint8_t sleepmode = 0;
volatile uint8_t region;
volatile uint8_t time_format;
extern volatile uint8_t screenmutex;
volatile uint8_t minute_changed = 0, hour_changed = 0, second_changed = 0;
volatile uint8_t score_mode_timeout = 0;
volatile uint8_t score_mode = SCORE_MODE_TIME;
volatile uint8_t last_score_mode;
volatile uint8_t displaystyle; //dataman - add access to displaystyle, enables MultiChron
volatile uint8_t RotateFlag;   //dataman - enables display rotation

#ifdef GPSENABLE
volatile uint8_t gpsenable=0;    //dataman - enables gps check
volatile int8_t timezone=0;     //timezone +/- gmt
#endif

// These store the current button states for all 3 buttons. We can 
// then query whether the buttons are pressed and released or pressed
// This allows for 'high speed incrementing' when setting the time
extern volatile uint8_t last_buttonstate, just_pressed, pressed;
extern volatile uint8_t buttonholdcounter;

extern volatile uint8_t timeoutcounter;

// How long we have been snoozing
uint16_t snoozetimer = 0;

SIGNAL(TIMER1_OVF_vect) {
  PIEZO_PORT ^= _BV(PIEZO);
}

volatile uint16_t millis = 0;
volatile uint16_t animticker, alarmticker;
SIGNAL(TIMER0_COMPA_vect) {
  if (millis)
    millis--;
  if (animticker)
    animticker--;

  if (alarming && !snoozetimer) {
    if (alarmticker == 0) {
      alarmticker = 300;
      if (TCCR1B == 0) {
	TCCR1A = 0; 
	TCCR1B =  _BV(WGM12) | _BV(CS10); // CTC with fastest timer
	TIMSK1 = _BV(TOIE1) | _BV(OCIE1A);
	OCR1A = (F_CPU / ALARM_FREQ) / 2;
      } else {
	TCCR1B = 0;
	// turn off piezo
	PIEZO_PORT &= ~_BV(PIEZO);
      }
    }
    alarmticker--;    
  }
}

extern uint8_t EE_ALARM_HOUR;
extern uint8_t EE_ALARM_MIN;
extern uint8_t EE_BRIGHT;
extern uint8_t EE_REGION;
extern uint8_t EE_TIME_FORMAT;
extern uint8_t EE_SNOOZE;
extern uint8_t EE_STYLE;


void init_eeprom(void) {	//Set eeprom to a default state.
 if (eeprom_read_byte(&EE_INIT) != EE_INITIALIZED) {
    DEBUG(putstring("Error with EEPROM data. Clock cannot function without it. Please reprogram.")); 
    DEBUG(uart_putw_dec(eeprom_read_byte(&EE_INIT))); DEBUG(putstring_nl(""));
    while(1) {
      beep(4000, 100);
      delay_ms(100);
      beep(4000, 100);
      delay_ms(100);
      beep(4000, 100);
      delay_ms(1000);
    }
  }
  //We do not have the capability to reinitialize the EEPROM, other than by reprogramming it.
  //Because of this, and because we are storing some data there, bad things will happen if the
  //eeprom is NOT initialized. This is why we error out with infinite triple beeps if it is
  //not initialized.
}

int main(void) {
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

  // setup uart
  uart_init(BRRL_192);
  DEBUGP("RATT Clock");

  // set up piezo
  PIEZO_DDR |= _BV(PIEZO);

  DEBUGP("clock!");
  clock_init();
  //beep(4000, 100);
  

  init_eeprom();
  
  region = eeprom_read_byte(&EE_REGION);
  time_format = eeprom_read_byte(&EE_TIME_FORMAT);
  DEBUGP("buttons!");
  initbuttons();

  setalarmstate();

  // setup 1ms timer on timer0
  TCCR0A = _BV(WGM01);
  TCCR0B = _BV(CS01) | _BV(CS00);
  OCR0A = 125;
  TIMSK0 |= _BV(OCIE0A);

  // turn backlight on
  DDRD |= _BV(3);
#ifndef BACKLIGHT_ADJUST
  PORTD |= _BV(3);
#else
  TCCR2A = _BV(COM2B1); // PWM output on pin D3
  TCCR2A |= _BV(WGM21) | _BV(WGM20); // fast PWM
  TCCR2B |= _BV(WGM22);
  OCR2A = OCR2A_VALUE;
  OCR2B = eeprom_read_byte(&EE_BRIGHT);
#endif

  DDRB |= _BV(5);
  beep(4000, 100);
  
  //glcdInit locks and disables interrupts in one of its functions.  If the LCD is not
  //plugged in, glcd will run forever.  For good reason, it would be desirable to know
  //that the LCD is plugged in and working correctly as a result.  This is why we are
  //using a watch dog timer.  The lcd should initialized in way less than 500 ms.
  wdt_enable(WDTO_2S);
  glcdInit();
  glcdClearScreen();

  
  //Dataman - InitiAmin now init displays(0) as well.
  //initdisplay(0);
  displaystyle = eeprom_read_byte(&EE_STYLE);
  RotateFlag = 0;
  initanim();
  
  while (1) {
    animticker = ANIMTICK_MS;

    // check buttons to see if we have interaction stuff to deal with
	if(just_pressed && alarming)
	{
	  just_pressed = 0;
	  setsnooze();
	}

	if(display_date==3 && !score_mode_timeout)
	{
		display_date=0;
		score_mode = SCORE_MODE_YEAR;
	    score_mode_timeout = SCORE_MODE_TIMEOUT;
	    //drawdisplay();
	}
#ifdef OPTION_DOW_DATELONG
	else if(display_date==2 && !score_mode_timeout)
	{
		display_date=3;
		score_mode = SCORE_MODE_DATE;
	    score_mode_timeout = SCORE_MODE_TIMEOUT;
	    //drawdisplay();
	}
	else if(display_date==1 && !score_mode_timeout)
	{
		display_date=4;
		score_mode = SCORE_MODE_DATELONG_MON;
	    score_mode_timeout = SCORE_MODE_TIMEOUT;
	    //drawdisplay();
	}
	else if(display_date==4 && !score_mode_timeout)
	{
		display_date=3;
		score_mode = SCORE_MODE_DATELONG_DAY;
		score_mode_timeout = SCORE_MODE_TIMEOUT;
	}
#endif
	
	/*if(display_date && !score_mode_timeout)
	{
	  if(last_score_mode == SCORE_MODE_DATELONG)
	  {
	    score_mode = SCORE_MODE_DOW;
	    score_mode_timeout = SCORE_MODE_TIMEOUT;
	    setscore();
	  }
	  
	  else if(last_score_mode == SCORE_MODE_DOW)
	  {
	    score_mode = SCORE_MODE_DATE;
	    score_mode_timeout = SCORE_MODE_TIMEOUT;
	    setscore();
	  }
	  else if(last_score_mode == SCORE_MODE_DATE)
	  {
	    score_mode = SCORE_MODE_YEAR;
	    score_mode_timeout = SCORE_MODE_TIMEOUT;
	    setscore();
	    display_date = 0;
	  }
	  
	}*/
	/*if(display_date && !score_mode_timeout)
	{
	  score_mode = SCORE_MODE_YEAR;
	  score_mode_timeout = SCORE_MODE_TIMEOUT;
	  setscore();
	  display_date = 0;
	}*/

    //Was formally set for just the + button.  However, because the Set button was never
    //accounted for, If the alarm was turned on, and ONLY the set button was pushed since then,
    //the alarm would not sound at alarm time, but go into a snooze immediately after going off.
    //This could potentially make you late for work, and had to be fixed.
	if (just_pressed & 0x6) {
	  just_pressed = 0;
#ifdef OPTION_DOW_DATELONG
	  if((region == REGION_US) || (region == REGION_EU)) {
#endif
	  	display_date = 3;
	  	score_mode = SCORE_MODE_DATE;
#ifdef OPTION_DOW_DATELONG
	  }
	  else if ((region == DOW_REGION_US) || (region == DOW_REGION_EU)) {
	  	display_date = 2;
	  	score_mode = SCORE_MODE_DOW;
	  }
	  else if (region == DATELONG) {
	  	display_date = 4;
	  	score_mode = SCORE_MODE_DATELONG_MON;
	  }
	  else {
	  	display_date = 1;
	  	score_mode = SCORE_MODE_DOW;
	  }
#endif
	  score_mode_timeout = SCORE_MODE_TIMEOUT;
	  //drawdisplay();
	}

    if (just_pressed & 0x1) {
      just_pressed = 0;
      display_date = 0;
      score_mode = SCORE_MODE_TIME;
      score_mode_timeout = 0;
      //drawdisplay();
      switch(displaymode) {
      case (SHOW_TIME):
	// DATAMAN - ADD STYLE MENU
	displaymode = SET_STYLE;
	set_style();
	break; 
	case SET_STYLE:
	// END ADD STYLE MENU
	displaymode = SET_ALARM;
	set_alarm();
	break;
      case (SET_ALARM):
	displaymode = SET_TIME;
	set_time();
	timeunknown = 0;
	break;
      case SET_TIME:
	displaymode = SET_DATE;
	set_date();
	break;
      case SET_DATE:
	displaymode = SET_REGION;
	set_region();
	break;
#ifdef BACKLIGHT_ADJUST
	  case SET_REGION:
	displaymode = SET_BRIGHTNESS;
	set_backlight();
	break;
#endif
      default:
	displaymode = SHOW_TIME;
	//glcdClearScreen();
	//Dataman - Changing initdisplays to initanims, need to make sure as animation may have changed.
	//initdisplay(0);
	//initanim();
      }

      if (displaymode == SHOW_TIME) {
	glcdClearScreen();
	//Dataman - Changing initdisplays to initanims, need to make sure as animation may have changed.
	//initdisplay(0);
	initanim();
      }
    }

    step();
    if (displaymode == SHOW_TIME) {
      if (! inverted && alarming && (time_s & 0x1)) {
	inverted = 1;
	initdisplay(inverted);
      }
      else if ((inverted && ! alarming) || (alarming && inverted && !(time_s & 0x1))) {
	inverted = 0;
	initdisplay(0);
      } else {
	PORTB |= _BV(5);
	drawdisplay(inverted);
	PORTB &= ~_BV(5);
      }
    }
  
    while (animticker);
    //uart_getchar();  // you would uncomment this so you can manually 'step'
  }

  halt();
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
  delay_ms(duration);
  TCCR1B = 0;
  // turn off piezo
  PIEZO_PORT &= ~_BV(PIEZO);
}

// This turns on/off the alarm when the switch has been
// set. It also displays the alarm time
void setalarmstate(void) {
  DEBUGP("a");
  if (ALARM_PIN & _BV(ALARM)) { 
    if (alarm_on) {
      // turn off the alarm
      alarm_on = 0;
      alarm_tripped = 0;
      snoozetimer = 0;
      if (alarming) {
	// if the alarm is going off, we should turn it off
	// and quiet the speaker
	DEBUGP("alarm off");
	alarming = 0;
	TCCR1B = 0;
	// turn off piezo
	PIEZO_PORT &= ~_BV(PIEZO);
      } 
    }
  } else {
    // Don't display the alarm/beep if we already have
    if  (!alarm_on) {
      // alarm on!
      alarm_on = 1;
      // reset snoozing
      snoozetimer = 0;
	  score_mode = SCORE_MODE_ALARM;
	  score_mode_timeout = SCORE_MODE_TIMEOUT;
	  //drawdisplay();
      DEBUGP("alarm on");
    }   
  }
}


void drawArrow(uint8_t x, uint8_t y, uint8_t l) {
  glcdFillRectangle(x, y, l, 1, ON);
  glcdSetDot(x+l-2,y-1);
  glcdSetDot(x+l-2,y+1);
  glcdSetDot(x+l-3,y-2);
  glcdSetDot(x+l-3,y+2);
}


void printnumber(uint8_t n, uint8_t inverted) {
  glcdWriteChar(n/10+'0', inverted);
  glcdWriteChar(n%10+'0', inverted);
}

uint8_t readi2ctime(void) {
  uint8_t regaddr = 0, r;
  uint8_t clockdata[8];
  
  // check the time from the RTC
  cli();
  r = i2cMasterSendNI(0xD0, 1, &regaddr);

  if (r != 0) {
    DEBUG(putstring("Reading i2c data: ")); DEBUG(uart_putw_dec(r)); DEBUG(putstring_nl(""));
    while(1) {
      beep(4000, 100);
      delay_ms(100);
      beep(4000, 100);
      delay_ms(1000);
    }
  }

  r = i2cMasterReceiveNI(0xD0, 7, &clockdata[0]);
  sei();

  if (r != 0) {
    DEBUG(putstring("Reading i2c data: ")); DEBUG(uart_putw_dec(r)); DEBUG(putstring_nl(""));
    while(1) {
      beep(4000, 100);
      delay_ms(100);
      beep(4000, 100);
      delay_ms(1000);
    }
  }

  time_s = ((clockdata[0] >> 4) & 0x7)*10 + (clockdata[0] & 0xF);
  time_m = ((clockdata[1] >> 4) & 0x7)*10 + (clockdata[1] & 0xF);
  if (clockdata[2] & _BV(6)) {
    // "12 hr" mode
    time_h = ((clockdata[2] >> 5) & 0x1)*12 + 
      ((clockdata[2] >> 4) & 0x1)*10 + (clockdata[2] & 0xF);
  } else {
    time_h = ((clockdata[2] >> 4) & 0x3)*10 + (clockdata[2] & 0xF);
  }
  
  date_d = ((clockdata[4] >> 4) & 0x3)*10 + (clockdata[4] & 0xF);
  date_m = ((clockdata[5] >> 4) & 0x1)*10 + (clockdata[5] & 0xF);
  date_y = ((clockdata[6] >> 4) & 0xF)*10 + (clockdata[6] & 0xF);

  return clockdata[0] & 0x80;
}

void writei2ctime(uint8_t sec, uint8_t min, uint8_t hr, uint8_t day,
		  uint8_t date, uint8_t mon, uint8_t yr) {
  uint8_t clockdata[8] = {0,0,0,0,0,0,0,0};

  clockdata[0] = 0; // address
  clockdata[1] = i2bcd(sec);  // s
  clockdata[2] = i2bcd(min);  // m
  clockdata[3] = i2bcd(hr); // h
  clockdata[4] = i2bcd(day);  // day
  clockdata[5] = i2bcd(date);  // date
  clockdata[6] = i2bcd(mon);  // month
  clockdata[7] = i2bcd(yr); // year
  
  cli();
  uint8_t r = i2cMasterSendNI(0xD0, 8, &clockdata[0]);
  sei();

  //DEBUG(putstring("Writing i2c data: ")); DEBUG(uart_putw_dec()); DEBUG(putstring_nl(""));

  if (r != 0) {
    while(1) {
      beep(4000, 100);
      delay_ms(100);
      beep(4000, 100);
      delay_ms(1000);
    }
  }

}

// runs at about 30 hz
uint8_t t2divider1 = 0, t2divider2 = 0;
SIGNAL (TIMER2_OVF_vect) {
  wdt_reset();
#ifdef BACKLIGHT_ADJUST
  if (t2divider1 == TIMER2_RETURN) {
#else
  if (t2divider1 == 5) {
#endif
    t2divider1 = 0;
  } else {
    t2divider1++;
    return;
  }

  //This occurs at 6 Hz

  uint8_t last_s = time_s;
  uint8_t last_m = time_m;
  uint8_t last_h = time_h;

  readi2ctime();
  
  if (time_h != last_h) {
    hour_changed = 1; 
    old_h = last_h;
    old_m = last_m;
  } else if (time_m != last_m) {
    minute_changed = 1;
    old_m = last_m;
  } else if (time_s != last_s) {
    second_changed = 1;
    old_s = last_s;
  }


  if (time_s != last_s) {
    if(alarming && snoozetimer)
	  snoozetimer--;

    if(score_mode_timeout) {
	  score_mode_timeout--;
	  if(!score_mode_timeout) {
	  	last_score_mode = score_mode;
	    score_mode = SCORE_MODE_TIME;
	    if(hour_changed) {
	      time_h = old_h;
	      time_m = old_m;
	    } else if (minute_changed) {
	      time_m = old_m;
	    }
	    if(hour_changed || minute_changed) {
	      time_h = last_h;
	      time_m = last_m;
	    }
	  }
	}


    DEBUG(putstring("**** "));
    DEBUG(uart_putw_dec(time_h));
    DEBUG(uart_putchar(':'));
    DEBUG(uart_putw_dec(time_m));
    DEBUG(uart_putchar(':'));
    DEBUG(uart_putw_dec(time_s));
    DEBUG(putstring_nl("****"));
  }

  if (((displaymode == SET_ALARM) ||
       (displaymode == SET_DATE) ||
       (displaymode == SET_REGION) ||
       (displaymode == SET_BRIGHTNESS)) &&
      (!screenmutex) ) {
      print_time(time_h, time_m, time_s, SET_TIME);
  }

  // check if we have an alarm set
  if (alarm_on && (time_s == 0) && (time_m == alarm_m) && (time_h == alarm_h)) {
    DEBUG(putstring_nl("ALARM TRIPPED!!!"));
    alarm_tripped = 1;
  }
  
  //And wait till the score changes to actually set the alarm off.
  if(!minute_changed && !hour_changed && alarm_tripped) {
  	 DEBUG(putstring_nl("ALARM GOING!!!!"));
  	 alarming = 1;
  	 alarm_tripped = 0;
  }

  if (t2divider2 == 6) {
    t2divider2 = 0;
  } else {
    t2divider2++;
    return;
  }

  if (buttonholdcounter) {
    buttonholdcounter--;
  }

  if (timeoutcounter) {
    timeoutcounter--;
  }
}

uint8_t leapyear(uint16_t y) {
  return ( (!(y % 4) && (y % 100)) || !(y % 400));
}

void tick(void) {


}



inline uint8_t i2bcd(uint8_t x) {
  return ((x/10)<<4) | (x%10);
}


void clock_init(void) {
  // talk to clock
  i2cInit();


  if (readi2ctime()) {
    DEBUGP("uh oh, RTC was off, lets reset it!");
    writei2ctime(0, 0, 12, 0, 1, 1, 9); // noon 1/1/2009
   }

  readi2ctime();

  DEBUG(putstring("\n\rread "));
  DEBUG(uart_putw_dec(time_h));
  DEBUG(uart_putchar(':'));
  DEBUG(uart_putw_dec(time_m));
  DEBUG(uart_putchar(':'));
  DEBUG(uart_putw_dec(time_s));

  DEBUG(uart_putchar('\t'));
  DEBUG(uart_putw_dec(date_d));
  DEBUG(uart_putchar('/'));
  DEBUG(uart_putw_dec(date_m));
  DEBUG(uart_putchar('/'));
  DEBUG(uart_putw_dec(date_y));
  DEBUG(putstring_nl(""));

  alarm_m = eeprom_read_byte(&EE_ALARM_MIN) % 60;
  alarm_h = eeprom_read_byte(&EE_ALARM_HOUR) % 24;


  //ASSR |= _BV(AS2); // use crystal

  TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20); // div by 1024
  // overflow ~30Hz = 8MHz/(255 * 1024)

  // enable interrupt
  TIMSK2 = _BV(TOIE2);

  sei();
}

void setsnooze(void) {
  //snoozetimer = eeprom_read_byte(&EE_SNOOZE);
  //snoozetimer *= 60; // convert minutes to seconds
  snoozetimer = MAXSNOOZE;
  TCCR1B = 0;
  // turn off piezo
  PIEZO_PORT &= ~_BV(PIEZO);
  DEBUGP("snooze");
  //displaymode = SHOW_SNOOZE;
  //delay_ms(1000);
  displaymode = SHOW_TIME;
}

#ifdef GPSENABLE

char uart_getch(void);
uint8_t GPSRead(uint8_t);
uint8_t DecodeGPSBuffer(char *t);

uint8_t GPSRead(uint8_t debugmode) {
 // debugmode 0=quiet, 1=debug to line 6 (used by anim_gps.c)
 // method, read chars dump to screen
 static uint8_t soh=0;
 static uint8_t blen=0;
 static char buffer[11];
 static int8_t dadjflag =0;
 static uint8_t scrpos=0;
 char ch=0;
 //                     JA FE MA AP MA JU JL AU SE OC NO DE
 uint8_t monthmath[] = {31,27,31,30,31,30,31,31,30,31,30,31};
 ch = uart_getch();
 if (ch<32) return 0;
 if (debugmode) {
  glcdSetAddress(6 * scrpos++, 6); 
  glcdPutCh(ch, NORMAL); 
  glcdPutCh(32, NORMAL); 
  if (++scrpos>21) {scrpos=0;}
 }
 // Check for start of sentence
 if (ch=='$') {
  soh=1; 
  blen=0; 
  return 0; 
 }
 // If inside a sentence...
 if (soh>0) {

  // check for next field
  if (ch == ',') {
   soh++; 
   if (blen==0) {buffer[0]=0;}  
   blen=0;
  }
  // otherwise, add character to buffer
  else { 
   if (blen<10) {
    buffer[blen++]=ch;
    buffer[blen]=0;
   } 
  }

  // Process: Command
  if (soh==2) {
   if (!strcmp(buffer,"GPRMC")) {soh=3; blen=0; buffer[0]=0;}
   else {soh=0;}
   return 0;
  }
  
  // Process: Time
  if (soh==4) { // Time Word
   soh++;
   if (debugmode) {
    buffer[6]=0; 
    glcdSetAddress(MENU_INDENT+42, 6); 
    glcdPutStr(buffer, NORMAL); 
    return 0;
   }
   time_s = DecodeGPSBuffer((char *)&buffer[4]); 
   time_m = DecodeGPSBuffer((char *)&buffer[2]);
   time_h = DecodeGPSBuffer(buffer);
   // Adjust hour by time zone offset
   // have to be careful because uint8's underflow back to 255, not -1
   dadjflag =0;
   if (timezone<0 && abs(timezone) > time_h) {
    dadjflag=-1; // Remind us to subtract a day... 
    time_h = 24 + time_h + timezone;
   }
   else {
    time_h+=timezone;
    if (time_h>24) { // Remind us to add a day...
     time_h-=24; 
     dadjflag=1;
    }
   }  
   return 0;
  }
  
  // Process: Date
  if (soh==13) {// Date Word
   if (debugmode) {
    glcdSetAddress(MENU_INDENT+82, 6); 
    glcdPutStr(buffer, NORMAL); 
    soh=0; 
    return 1;
   }
   // Joy, datemath...
   date_d = DecodeGPSBuffer(buffer);  
   date_m = DecodeGPSBuffer((char *)&buffer[2]); 
   date_y = DecodeGPSBuffer((char *)&buffer[4]);
   // dadjflag is set by the time routine to remember to add or subtract a day...
   if (dadjflag) {
    date_d += dadjflag;
    if (!date_d) { // Subtracted to Day=0
     if (!--date_m) {
      date_y--;
      date_m = 12;
      date_d = 31;
     }
     else {
     date_d = monthmath[date_m-1];
     }     
    }
    else { // check for date > end of month (including leap year calc)
     if (date_d > (monthmath[date_m-1] + (date_m == 2 && (date_y%4)==0 ? 1 : 0))) {
      date_d = 1;
      date_m++;
      if (date_m>12) {
       date_y++; 
       date_m=1;
      }
     }
    }
   }
   // Save Results
   writei2ctime(time_s, time_m, time_h, dotw(date_m, date_d, date_y), date_d, date_m, date_y);
   // Do we need to set hourchanged, minutechanged, etc?
   return 1;
  }
 }
 return 0;
}

// Decodes a 2 char number to uint8
uint8_t DecodeGPSBuffer(char *cBuffer) {
 return ((cBuffer[0]-48)*10) + (cBuffer[1]-48);
}

#endif

