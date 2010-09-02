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

uint8_t init_set_menu(uint8_t line)
{
  display_menu();
  
  screenmutex++;
#ifdef BACKLIGHT_ADJUST
  if(displaymode == SET_BRIGHTNESS)
#else
  if(displaymode == SET_REGION)
#endif
  	  print_menu_exit();
  // put a small arrow next to 'set 12h/24h'
  drawArrow(0, (line*8)+3, MENU_INDENT -1);
  screenmutex--;
   
  timeoutcounter = INACTIVITYTIMEOUT;
  return displaymode;
}


//Dataman - Handle setting style
void set_style(void) {
  displaystyle = eeprom_read_byte(&EE_STYLE);
  uint8_t mode = init_set_menu(0);
  while (!check_timeout()) {
  
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
		if (displaystyle<=STYLE_ROTATE) eeprom_write_byte(&EE_STYLE,displaystyle);
        return;
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_STL) {
	    displaystyle ++;
	    if (displaystyle>STYLE_ABOUT) displaystyle=STYLE_BASE + 1;
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

void print_number_slash(uint8_t number, uint8_t inverted)
{
	printnumber(number, inverted);
	glcdWriteChar('/', NORMAL);
}
#ifdef OPTION_DOW_DATELONG
  #define MAX_ORDER 6
#else
  #define MAX_ORDER 2
#endif

void print_date(uint8_t month, uint8_t day, uint8_t year, uint8_t mode) {
  glcdSetAddress(MENU_INDENT, 3);
  glcdPutStr("Date:", NORMAL);
  if (region == REGION_US) {
  	glcdPutStr("     ",NORMAL);
  	print_number_slash(month,(mode == SET_MONTH)?INVERTED:NORMAL);
  	print_number_slash(day, (mode == SET_DAY)?INVERTED:NORMAL);
  } else if (region == REGION_EU) {
  	glcdPutStr("     ",NORMAL);
  	print_number_slash(day, (mode == SET_DAY)?INVERTED:NORMAL);
  	print_number_slash(month,(mode == SET_MONTH)?INVERTED:NORMAL);
  }
#ifdef OPTION_DOW_DATELONG 
  else if ( region == DOW_REGION_US) {
  	glcdWriteChar(' ', NORMAL);
  	print_dow(NORMAL,month,day,year);
  	print_number_slash(month,(mode == SET_MONTH)?INVERTED:NORMAL);
  	print_number_slash(day, (mode == SET_DAY)?INVERTED:NORMAL);
  } else if ( region == DOW_REGION_EU) {
  	glcdWriteChar(' ', NORMAL);
  	print_dow(NORMAL,month,day,year);
  	print_number_slash(day, (mode == SET_DAY)?INVERTED:NORMAL);
  	print_number_slash(month,(mode == SET_MONTH)?INVERTED:NORMAL);
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

void print_monthday_help(uint8_t mode)
{
	if(mode == SET_MONTH)
		print_menu_opts("change mon","set mon.");
	else if (mode == SET_DAY)
		print_menu_opts("change day","set date");
	else if (mode == SET_YEAR)
		print_menu_opts("change yr.","set year");
	else
		print_menu_advance();
}

//Code optimization for set date / set deathchron date of birth.
uint8_t next_mode_setdate[MAX_ORDER] = {
	SET_MONTH,
	SET_DAY,
#ifdef OPTION_DOW_DATELONG
	SET_MONTH,
	SET_DAY,
	SET_MONTH,
	SET_MONTH
#endif
};

uint8_t next_mode_setmonth[MAX_ORDER] = {
	SET_DAY,
	SET_YEAR,
#ifdef OPTION_DOW_DATELONG
	SET_DAY,
	SET_YEAR,
	SET_DAY,
	SET_DAY
#endif
};

uint8_t next_mode_setday[MAX_ORDER] = {
	SET_YEAR,
	SET_MONTH,
#ifdef OPTION_DOW_DATELONG
	SET_YEAR,
	SET_MONTH,
	SET_YEAR,
	SET_YEAR
#endif
};

void set_date(void) {
  uint8_t mode = init_set_menu(3);
  uint8_t day, month, year;
    
  day = date_d;
  month = date_m;
  year = date_y;
  while (!check_timeout()) {
    
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_DATE) {
	DEBUG(putstring("Set date month/day, depending on region"));
	// ok now its selected
	mode = next_mode_setdate[region];
	
      } else if (mode == SET_MONTH) {
	DEBUG(putstring("Set date day/year, depending on region"));
	mode = next_mode_setmonth[region];
      } else if (mode == SET_DAY) {
	DEBUG(putstring("Set date month/year, depending on region"));
	mode = next_mode_setday[region];
      } else {
	// done!
	DEBUG(putstring("done setting date"));
	mode = SET_DATE;
	
	//Update the DS1307 with set date.
	date_y = year;
	date_m = month;
	date_d = day;
	writei2ctime(time_s, time_m, time_h, 0, date_d, date_m, date_y);
      }
      //Print the instructions below
      print_monthday_help(mode);
      //Refresh the date.
      print_date(month,day,year,mode);
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;

      screenmutex++;

      if (mode == SET_MONTH) {
      month++;
      }
      if (mode == SET_DAY) {
	day++;
      }
      if (mode == SET_YEAR) {
	year = (year+1) % 100;
      }
      add_month(&month, &day, year);
      print_date(month,day,year,mode);
      screenmutex--;

      if (pressed & 0x4)
	delay_ms(200);  
    }
  }

}

#ifdef BACKLIGHT_ADJUST
void set_backlight(void) {
  uint8_t mode = init_set_menu(5);
  while (!check_timeout()) {
    
  
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
  uint8_t mode = init_set_menu(4);

  while (!check_timeout()) {
    
  
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
		}
		time_format = !time_format;
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
  uint8_t mode = init_set_menu(1);

  while (!check_timeout()) {
    
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
	delay_ms(200);
    }
  }
}

void set_time(void) {
  uint8_t mode = init_set_menu(2);

  uint8_t hour, min, sec;
    
  hour = time_h;
  min = time_m;
  sec = time_s;

  while (!check_timeout()) {
    
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
	delay_ms(200);
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
#ifdef RATTCHRON
  case STYLE_RAT: glcdPutStr("RATTChron",inverted);
                break;
#endif
#ifdef INTRUDERCHRON
  case STYLE_INT: glcdPutStr("IntruderChron",inverted);
				break;
#endif
#ifdef SEVENCHRON
  case STYLE_SEV: glcdPutStr("SevenChron",inverted);
 				break;
#endif
#ifdef XDALICHRON
  case STYLE_XDA: glcdPutStr("XDALIChron",inverted);
                 break;
#endif
#ifdef TSCHRON
  case STYLE_TS: glcdPutStr("TimesSqChron",inverted);
                 break;
#endif
#ifdef DEATHCHRON
  case STYLE_DEATH: glcdPutStr("DeathChron", inverted);
                break;
#endif
  case STYLE_RANDOM: glcdPutStr("Random",inverted);
  				break;
  case STYLE_ROTATE: glcdPutStr("Rotate",inverted);
  				break;
#ifdef DEATHCHRON
  case STYLE_DEATHCFG: glcdPutStr("DeathChron Cfg", inverted);
                break;
#endif
#ifdef GPSENABLE
  case STYLE_GPS: glcdPutStr("GPS Setup",inverted);
  				break;
#endif
  case STYLE_ABOUT:  glcdPutStr("About",inverted);
  				break;
  }
}

