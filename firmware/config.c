#include <avr/io.h>      // this contains all the IO port definitions
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <string.h>
#include "util.h"
#include "ratt.h"
#include "ks0108.h"
#include "glcd.h"

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarm_h, alarm_m;
extern volatile uint8_t last_buttonstate, just_pressed, pressed;
extern volatile uint8_t buttonholdcounter;
extern volatile uint8_t region;
extern volatile uint8_t time_format;
extern volatile uint8_t alarm_on;

//dataman - add access to display style
extern volatile uint8_t displaystyle;
extern volatile uint8_t RotateFlag;

extern volatile uint8_t displaymode;
// This variable keeps track of whether we have not pressed any
// buttons in a few seconds, and turns off the menu display
volatile uint8_t timeoutcounter = 0;

volatile uint8_t screenmutex = 0;

//Prototypes
void print_style_setting(uint8_t inverted);
void print_region_setting(uint8_t inverted);
void print_menu_advance(void);
void print_menu_change(void);
void print_menu_exit(void);
void print_menu_opts(char *Opt1, char *Opt2);
void print_menu(char*, char*, char*, char*);
void print_menu_line(uint8_t line, char *Button, char *Opt);
//

void print_alarmline(uint8_t mode)
{
  glcdSetAddress(MENU_INDENT, 1);
  glcdPutStr("Set Alarm:  ", NORMAL);
  print_alarmhour(alarm_h, ((mode==SET_HOUR)?INVERTED:NORMAL));
  glcdWriteChar(':', NORMAL);
  printnumber(alarm_m, ((mode==SET_MIN)?INVERTED:NORMAL));
}

void print_time(uint8_t hour, uint8_t min, uint8_t sec, uint8_t mode)
{
  glcdSetAddress(MENU_INDENT, 2);
  glcdPutStr("Set Time: ", NORMAL);
  print_timehour(hour, ((mode==SET_HOUR)?INVERTED:NORMAL));
  glcdWriteChar(':', NORMAL);
  printnumber(min, ((mode==SET_MIN)?INVERTED:NORMAL));
  glcdWriteChar(':', NORMAL);
  printnumber(sec, ((mode==SET_SEC)?INVERTED:NORMAL));
  if (time_format == TIME_12H) {
    glcdWriteChar(' ', NORMAL);
    if (hour >= 12) {
      glcdWriteChar('P', ((mode==SET_HOUR)?INVERTED:NORMAL));
    } else {
      glcdWriteChar('A', ((mode==SET_HOUR)?INVERTED:NORMAL));
    }
  }
}

void print_backlight(uint8_t mode)
{
  glcdSetAddress(MENU_INDENT, 5);
  glcdPutStr("Set Backlight: ", NORMAL);
  printnumber(OCR2B>>OCR2B_BITSHIFT,((mode==SET_BRT)?INVERTED:NORMAL));
}

void display_menu(void) {
  DEBUGP("display menu");
  
  screenmutex++;

  glcdClearScreen();
  
  //Dataman - Mode Menu Option
  //glcdSetAddress(0, 0);
  //glcdPutStr("Configuration Menu", NORMAL);
  glcdSetAddress(MENU_INDENT, 0);
  glcdPutStr("Mode:", NORMAL);
  print_style_setting(NORMAL);
 
  print_alarmline(SET_ALARM);
  
  print_time(time_h,time_m,time_s,SET_TIME);
  print_date(date_m,date_d,date_y,SET_DATE);
  print_region_setting(NORMAL);
  
#ifdef BACKLIGHT_ADJUST
  print_backlight(SET_BRIGHTNESS);
#endif
  
  print_menu_advance();

  screenmutex--;
}


//Dataman - Handle setting style
void set_style(void) {
  uint8_t mode = SET_STYLE;

  display_menu();
  
  screenmutex++;
  print_menu_advance();
 
  // put a small arrow next to 'set 12h/24h'
  drawArrow(0, 3, MENU_INDENT -1);
  screenmutex--;
   
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
  
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_STYLE) {
	DEBUG(putstring("Setting mode"));
	// ok now its selected
	mode = SET_STL;
	// print the region 
	print_style_setting(INVERTED);
 	
	// display instructions below
	print_menu_change();
      } else {
	mode = SET_STYLE;
	// print the region normal
	print_style_setting(NORMAL);
	print_menu_advance();
        // faster return?
        RotateFlag = 0;
        displaymode = SHOW_TIME;
        return;
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_STL) {
	    displaystyle ++;
	    if (displaystyle>STYLE_ABOUT) displaystyle=STYLE_INT;
	    if (displaystyle<STYLE_GPS) eeprom_write_byte(&EE_STYLE,displaystyle);
	screenmutex++;
	display_menu();
	print_menu_change();

	// put a small arrow next to 'set 12h/24h'
	drawArrow(0, 3, MENU_INDENT -1);
	print_style_setting(INVERTED);
 	
	screenmutex--;

	//eeprom_write_byte(&EE_BRIGHT, OCR2B);
      }
    }
  }
}


#ifdef OPTION_DOW_DATELONG
void print_month(uint8_t inverted, uint8_t month) {
  glcdWriteChar(smon(month,0),inverted);
  glcdWriteChar(smon(month,1),inverted);
  glcdWriteChar(smon(month,2),inverted);
}

void print_dow(uint8_t inverted, uint8_t mon, uint8_t day, uint8_t yr) {
 uint8_t t = dotw(mon,day,yr);
 glcdWriteChar(sdotw(t,0),inverted);
 glcdWriteChar(sdotw(t,1),inverted);
 glcdWriteChar(sdotw(t,2),inverted);
}  
#endif

void print_date(uint8_t month, uint8_t day, uint8_t year, uint8_t mode) {
  glcdSetAddress(MENU_INDENT, 3);
  glcdPutStr("Date:", NORMAL);
  if (region == REGION_US) {
  	glcdPutStr("     ",NORMAL);
    printnumber(month, (mode == SET_MONTH)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
  } else if (region == REGION_EU) {
  	glcdPutStr("     ",NORMAL);
    printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber(month, (mode == SET_MONTH)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
  }
#ifdef OPTION_DOW_DATELONG 
  else if ( region == DOW_REGION_US) {
  	glcdWriteChar(' ', NORMAL);
  	print_dow(NORMAL,month,day,year);
  	printnumber(month, (mode == SET_MONTH)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
  } else if ( region == DOW_REGION_EU) {
  	glcdWriteChar(' ', NORMAL);
  	print_dow(NORMAL,month,day,year);
  	printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber(month, (mode == SET_MONTH)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
  } else if ( region == DATELONG) {
  	glcdPutStr("   ",NORMAL);
  	print_month((mode == SET_MONTH)?INVERTED:NORMAL,month);
  	glcdWriteChar(' ', NORMAL);
  	printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
  	glcdWriteChar(',', NORMAL);
  	glcdWriteChar(' ', NORMAL);
  } else {
  	print_dow(NORMAL,month,day,year);
  	print_month((mode == SET_MONTH)?INVERTED:NORMAL,month);
  	glcdWriteChar(' ', NORMAL);
  	printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
  	glcdWriteChar(',', NORMAL);
  }
#endif
  printnumber(20,(mode == SET_YEAR)?INVERTED:NORMAL);
  printnumber(year, (mode == SET_YEAR)?INVERTED:NORMAL);
}

void set_date(void) {
  uint8_t mode = SET_DATE;
  uint8_t day, month, year;
    
  day = date_d;
  month = date_m;
  year = date_y;

  display_menu();

  screenmutex++;
  // put a small arrow next to 'set date'
  drawArrow(0, 27, MENU_INDENT -1);
  screenmutex--;
  
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

#ifdef OPTION_DOW_DATELONG
      if ((mode == SET_DATE) && ((region == REGION_US) || (region == DOW_REGION_US) || (region == DATELONG) || (region == DATELONG_DOW))) {
#else
      if ((mode == SET_DATE) && (region == REGION_US)) {
#endif
	DEBUG(putstring("Set date month"));
	// ok now its selected
	mode = SET_MONTH;

	// print the month inverted
	// display instructions below
	print_menu_opts("change mon","set mon.");
      } else if ((mode == SET_DATE) && ((region == REGION_EU) || (region == DOW_REGION_EU))) {
	DEBUG(putstring("Set date month"));
	// ok now its selected
	mode = SET_DAY;

	// print the day inverted
	// display instructions below
	print_menu_opts("change day","set date");
#ifdef OPTION_DOW_DATELONG
      } else if ((mode == SET_MONTH) && ((region == REGION_US) || (region == DOW_REGION_US) || (region == DATELONG) || (region == DATELONG_DOW))) {
#else
      } else if ((mode == SET_MONTH) && (region == REGION_US)) {
#endif
	DEBUG(putstring("Set date day"));
	mode = SET_DAY;

	// display instructions below
	print_menu_opts("change day","set date");
      }else if ((mode == SET_DAY) && ((region == REGION_EU) || (region == DOW_REGION_EU))) {
	DEBUG(putstring("Set date month"));
	mode = SET_MONTH;

	// display instructions below
	print_menu_opts("change mon","set mon.");
#ifdef OPTION_DOW_DATELONG
      } else if ( ((mode == SET_DAY) && ((region == REGION_US) || (region == DOW_REGION_US) || (region == DATELONG) || (region == DATELONG_DOW))) ||
		  ((mode == SET_MONTH) && ((region == REGION_EU) || (region == DOW_REGION_EU))) )  {
#else
      } else if ( ((mode == SET_DAY) && (region == REGION_US)) ||
		  ((mode == SET_MONTH) && (region == REGION_EU)) )  {
#endif
	DEBUG(putstring("Set year"));
	mode = SET_YEAR;
	// print the date normal

	// display instructions below
	print_menu_opts("change yr.","set year");
      } else {
	// done!
	DEBUG(putstring("done setting date"));
	mode = SET_DATE;
	// print the seconds normal
	// display instructions below
	print_menu_advance();
	
	date_y = year;
	date_m = month;
	date_d = day;
	writei2ctime(time_s, time_m, time_h, 0, date_d, date_m, date_y);
      }
      print_date(month,day,year,mode);
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;

      screenmutex++;

      if (mode == SET_MONTH) {
	month++;
	if (month >= 13)
	  month = 1;
	if(month == 2) {
	  if(leapyear(year) && (day > 29))
	  	day = 29;
	  else if (!leapyear(year) && (day > 28))
	  	day = 28;
	} else if ((month == 4) || (month == 6) || (month == 9) || (month == 11)) {
      if(day > 30)
      	day = 30;
	}
	
      }
      if (mode == SET_DAY) {
	day++;
	if (day > 31)
	  day = 1;
	if(month == 2) {
	  if(leapyear(year) && (day > 29))
	  	day = 1;
	  else if (!leapyear(year) && (day > 28))
	  	day = 1;
	} else if ((month == 4) || (month == 6) || (month == 9) || (month == 11)) {
      if(day > 30)
      	day = 1;
	}
      }
      if (mode == SET_YEAR) {
	year = (year+1) % 100;
      }
      print_date(month,day,year,mode);
      screenmutex--;

      if (pressed & 0x4)
	_delay_ms(200);  
    }
  }

}

#ifdef BACKLIGHT_ADJUST
void set_backlight(void) {
  uint8_t mode = SET_BRIGHTNESS;

  display_menu();
  
  screenmutex++;
  print_menu_exit();

  // put a small arrow next to 'set 12h/24h'
  drawArrow(0, 43, MENU_INDENT -1);
  screenmutex--;
  
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
  
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_BRIGHTNESS) {
	DEBUG(putstring("Setting backlight"));
	// ok now its selected
	mode = SET_BRT;
	// print the region 
	// display instructions below
	print_menu_change();

      } else {
	mode = SET_BRIGHTNESS;
	// print the region normal
        print_menu_exit();
      }
      print_backlight(mode);
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_BRT) {
	    OCR2B += OCR2B_PLUS;
	    if(OCR2B > OCR2A_VALUE)
	      OCR2B = 0;
	screenmutex++;
	print_backlight(mode);
	screenmutex--;

	eeprom_write_byte(&EE_BRIGHT, OCR2B);
      }
    }
  }
}
#endif

void print_region_setting(uint8_t inverted) {
  glcdSetAddress(MENU_INDENT, 4);
  glcdPutStr("Region: ", NORMAL);
  if ((region == REGION_US) && (time_format == TIME_12H)) {
    glcdPutStr("     US 12hr", inverted);
  } else if ((region == REGION_US) && (time_format == TIME_24H)) {
    glcdPutStr("     US 24hr", inverted);
  } else if ((region == REGION_EU) && (time_format == TIME_12H)) {
    glcdPutStr("     EU 12hr", inverted);
  } else if ((region == REGION_EU) && (time_format == TIME_24H)){
    glcdPutStr("     EU 24hr", inverted);
  } 
#ifdef OPTION_DOW_DATELONG
  else if ((region == DOW_REGION_US) && (time_format == TIME_12H)) {
    glcdPutStr(" US 12hr DOW", inverted);
  } else if ((region == DOW_REGION_US) && (time_format == TIME_24H)) {
    glcdPutStr(" US 24hr DOW", inverted);
  } else if ((region == DOW_REGION_EU) && (time_format == TIME_12H)) {
    glcdPutStr(" EU 12hr DOW", inverted);
  } else if ((region == DOW_REGION_EU) && (time_format == TIME_24H)){
    glcdPutStr(" EU 24hr DOW", inverted);
  } else if ((region == DATELONG) && (time_format == TIME_12H)) {
    glcdPutStr("   12hr LONG", inverted);
  } else if ((region == DATELONG) && (time_format == TIME_24H)) {
    glcdPutStr("   24hr LONG", inverted);
  } else if ((region == DATELONG_DOW) && (time_format == TIME_12H)) {
    glcdPutStr("12h LONG DOW", inverted);
  } else if ((region == DATELONG_DOW) && (time_format == TIME_24H)){
    glcdPutStr("24h LONG DOW", inverted);
  }
#endif
}

void set_region(void) {
  uint8_t mode = SET_REGION;

  display_menu();
  
  screenmutex++;
  
#ifndef BACKLIGHT_ADJUST
  print_menu_exit();
#endif

  // put a small arrow next to 'set 12h/24h'
  drawArrow(0, 35, MENU_INDENT -1);
  screenmutex--;
  
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
  
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_REGION) {
	DEBUG(putstring("Setting region"));
	// ok now its selected
	mode = SET_REG;
	// print the region 
	print_region_setting(INVERTED);
	// display instructions below
	print_menu_change();
      } else {
	mode = SET_REGION;
	// print the region normal
	print_region_setting(NORMAL);
#ifdef BACKLIGHT_ADJUST
        print_menu_advance();
#else
	print_menu_exit();
#endif
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_REG) {
	    if(time_format) {        
	      region++;
#ifdef OPTION_DOW_DATELONG
	      if(region > DATELONG_DOW)
#else
          if(region > REGION_EU)
#endif
	        region = 0;
		  time_format = !time_format;
		} else {
		  time_format = !time_format;
		}
	screenmutex++;
	display_menu();
	print_menu_change();

	// put a small arrow next to 'set 12h/24h'
	drawArrow(0, 35, MENU_INDENT -1);

	print_region_setting(INVERTED);
	screenmutex--;

	eeprom_write_byte(&EE_REGION, region);
	eeprom_write_byte(&EE_TIME_FORMAT, time_format);    
      }
    }
  }
}

void set_alarm(void) {
  uint8_t mode = SET_ALARM;

  display_menu();
  screenmutex++;
  // put a small arrow next to 'set alarm'
  drawArrow(0, 11, MENU_INDENT -1);
  screenmutex--;
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_ALARM) {
	DEBUG(putstring("Set alarm hour"));
	// ok now its selected
	mode = SET_HOUR;
	// display instructions below
        print_menu_opts("change hr.","set hour");
      } else if (mode == SET_HOUR) {
	DEBUG(putstring("Set alarm min"));
	mode = SET_MIN;
	// print the hour normal
	// display instructions below
	print_menu_opts("change min","set mins");
      } else {
	mode = SET_ALARM;
	// print the hour normal
	// display instructions below
	print_menu_advance();
      }
      print_alarmline(mode);
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_HOUR) {
	alarm_h = (alarm_h+1) % 24;
	// print the hour inverted
	eeprom_write_byte(&EE_ALARM_HOUR, alarm_h);    
      }
      if (mode == SET_MIN) {
	alarm_m = (alarm_m+1) % 60;
	eeprom_write_byte(&EE_ALARM_MIN, alarm_m);    
      }
      print_alarmline(mode);
      screenmutex--;
      if (pressed & 0x4)
	_delay_ms(200);
    }
  }
}

void set_time(void) {
  uint8_t mode = SET_TIME;

  uint8_t hour, min, sec;
    
  hour = time_h;
  min = time_m;
  sec = time_s;

  display_menu();
  
  screenmutex++;
  // put a small arrow next to 'set time'
  drawArrow(0, 19, MENU_INDENT -1);
  screenmutex--;
 
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_TIME) {
	DEBUG(putstring("Set time hour"));
	// ok now its selected
	mode = SET_HOUR;
	// display instructions below
        print_menu_opts("change hr","set hour");
      } else if (mode == SET_HOUR) {
	DEBUG(putstring("Set time min"));
	mode = SET_MIN;
	// display instructions below
        print_menu_opts("change min","set mins");
      } else if (mode == SET_MIN) {
	DEBUG(putstring("Set time sec"));
	mode = SET_SEC;
	// display instructions below
        print_menu_opts("sec","set secs");
      } else {
	// done!
	DEBUG(putstring("done setting time"));
	mode = SET_TIME;
	// display instructions below
	print_menu_advance();
	
	time_h = hour;
	time_m = min;
	time_s = sec;
	writei2ctime(time_s, time_m, time_h, 0, date_d, date_m, date_y);
      }
      print_time(hour,min,sec,mode);
      screenmutex--;
    }
    // was easter egg
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      screenmutex++;
      if (mode == SET_HOUR) {
	hour = (hour+1) % 24;
	time_h = hour;
      }
      if (mode == SET_MIN) {
	min = (min+1) % 60;
      }
      if (mode == SET_SEC) {
	sec = (sec+1) % 60;
      }
      print_time(hour,min,sec,mode);
      screenmutex--;
      if (pressed & 0x4)
	_delay_ms(200);
    }
  }
}

void print_timehour(uint8_t h, uint8_t inverted) {
  if (time_format == TIME_12H) {
    if (((h + 23)%12 + 1) >= 10 ) {
      printnumber((h + 23)%12 + 1, inverted);
    } else {
      glcdWriteChar(' ', NORMAL);
      glcdWriteChar('0' + (h + 23)%12 + 1, inverted);
    }
  } else {
    glcdWriteChar(' ', NORMAL);
    glcdWriteChar(' ', NORMAL);
    printnumber(h, inverted);
  }
}

void print_alarmhour(uint8_t h, uint8_t inverted) {
  if (time_format == TIME_12H) {
    glcdSetAddress(MENU_INDENT + 18*6, 1);
    if (h >= 12) 
      glcdWriteChar('P', NORMAL);
    else
      glcdWriteChar('A', NORMAL);
    glcdWriteChar('M', NORMAL);
    glcdSetAddress(MENU_INDENT + 12*6, 1);

    if (((h + 23)%12 + 1) >= 10 ) {
      printnumber((h + 23)%12 + 1, inverted);
    } else {
      glcdWriteChar(' ', NORMAL);
      glcdWriteChar('0' + (h + 23)%12 + 1, inverted);
    }
   } else {
    glcdSetAddress(MENU_INDENT + 12*6, 1);
    printnumber(h, inverted);
  }
}

void print_style_setting(uint8_t inverted) {
glcdSetAddress(43, 0);
  switch (displaystyle) {
  case STYLE_RAT: glcdPutStr("RATTChron",inverted);
                break;
  case STYLE_INT: glcdPutStr("IntruderChron",inverted);
				break;
  case STYLE_SEV: glcdPutStr("SevenChron",inverted);
 				break;
  case STYLE_XDA: glcdPutStr("XDALIChron",inverted);
                 break;
  case STYLE_RANDOM: glcdPutStr("Random",inverted);
  				break;
  case STYLE_ROTATE: glcdPutStr("Rotate",inverted);
  				break;
  case STYLE_GPS: glcdPutStr("GPS Setup",inverted);
  				break;
  case STYLE_ABOUT:  glcdPutStr("About",inverted);
  				break;
  }
}

void print_menu_advance(){
  print_menu("MENU","advance","SET","set");
  // Press MENU to avance
  // Press SET to set
}

void print_menu_exit(){
  print_menu("MENU","exit","SET","save");
  //Press MENU to exit
  //Press SET to set
}

void print_menu_change(){
 print_menu_opts("change","save");
 // Press + to change
 // Press SET to save
}

void print_menu_opts(char *Opt1, char *Opt2){
 print_menu("+",Opt1,"SET",Opt2);
 // Press + to X
 // Press SET to X
}

void print_menu(char *Button1, char *Opt1, char *Button2, char *Opt2){
 glcdFillRectangle(0, 48, GLCD_XPIXELS, 16, NORMAL);
 print_menu_line(6,Button1,Opt1);
 print_menu_line(7,Button2,Opt2);
}

void print_menu_line(uint8_t line, char *Button, char *Action){
  glcdSetAddress(0, line);
  glcdPutStr("Press ",NORMAL);
  glcdPutStr(Button,NORMAL);
  glcdPutStr(" to ",NORMAL);
  glcdPutStr(Action,NORMAL);
}





