/* ***************************************************************************
// config.c - the configuration menu handling
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
**************************************************************************** */

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
#include "deathclock.h"

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarm_h, alarm_m;
extern volatile uint8_t last_buttonstate, just_pressed, pressed;
extern volatile uint8_t buttonholdcounter;
extern volatile uint8_t region;
extern volatile uint8_t time_format;
extern volatile uint8_t border_tick;

volatile uint8_t cfg_dob_d, cfg_dob_m, cfg_dob_y, cfg_gender, cfg_dc_mode, cfg_bmi_unit, cfg_smoker;
volatile uint16_t cfg_bmi_height, cfg_bmi_weight;

extern volatile uint8_t displaymode;
// This variable keeps track of whether we have not pressed any
// buttons in a few seconds, and turns off the menu display
volatile uint8_t timeoutcounter = 0;

volatile uint8_t screenmutex = 0;

void deathclock_changed(void) //Any changes to the death clock neccesitates a recalculation of the death clock.
{
	uint8_t ee_set_year = date_y + 100;
	uint8_t max_year_diff[4][2] = {{72,78},{57,63},{82,88},{35,38}};
	
	if(((date_y + 100)-cfg_dob_y)>max_year_diff[cfg_dc_mode][cfg_gender]) ee_set_year = cfg_dob_y + max_year_diff[cfg_dc_mode][cfg_gender];
	
	eeprom_write_byte(&EE_SET_MONTH,date_m);
	eeprom_write_byte(&EE_SET_DAY,date_d);
	eeprom_write_byte(&EE_SET_YEAR,ee_set_year);
	eeprom_write_byte(&EE_SET_HOUR,time_h);
	eeprom_write_byte(&EE_SET_MIN,time_m);
	eeprom_write_byte(&EE_SET_SEC,time_s);
	load_etd();
}

void menu_advance_set_exit(uint8_t exit)
{
	glcdSetAddress(0, 6);
  if(!exit)
    glcdPutStr("Press MENU to advance", NORMAL);
  else
  	glcdPutStr("Press MENU to exit   ", NORMAL);
  glcdSetAddress(0, 7);
  glcdPutStr("Press SET to set     ", NORMAL);
}

#define PLUS_TO_CHANGE(x,y) plus_to_change(PSTR(x),PSTR(y))
void plus_to_change(const char *str1, const char *str2)
{
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change", NORMAL);
	glcdPutStr_rom(str1,NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to", NORMAL);
	glcdPutStr_rom(str2,NORMAL);
}

void plus_to_change_default(void)
{
	PLUS_TO_CHANGE("    "," save    ");
}

void display_dob(uint8_t inverted)
{
  glcdSetAddress(MENU_INDENT, 1);
  glcdPutStr("Set DOB:  ",NORMAL);
   if (region == REGION_US) {
    printnumber_2d(cfg_dob_m, inverted&1);
    glcdWriteChar('/', NORMAL);
    printnumber_2d(cfg_dob_d, inverted&2);
  } else {
    printnumber_2d(cfg_dob_d, inverted&2);
    glcdWriteChar('/', NORMAL);
    printnumber_2d(cfg_dob_m, inverted&1);
  }
  glcdWriteChar('/', NORMAL);
  printnumber_2d((cfg_dob_y+1900)/100, inverted&4);
  printnumber_2d((cfg_dob_y+1900)%100, inverted&4);
}

void display_gender(uint8_t inverted)
{
	glcdSetAddress(MENU_INDENT, 2);
  glcdPutStr("Set Gender:   ",NORMAL);
  if(cfg_gender==DC_gender_male)
  	  glcdPutStr("  Male", inverted);
  else
  	  glcdPutStr("Female", inverted);
}

void display_dc_mode(uint8_t inverted)
{
	glcdSetAddress(MENU_INDENT, 3);
  glcdPutStr("Set Mode:", NORMAL);
  if(cfg_dc_mode == DC_mode_normal)
    glcdPutStr("     Normal", inverted);
  else if (cfg_dc_mode == DC_mode_pessimistic)
    glcdPutStr("Pessimistic", inverted);
  else if (cfg_dc_mode == DC_mode_optimistic)
    glcdPutStr(" Optimistic", inverted);
  else
    glcdPutStr("   Sadistic", inverted);
}

void display_bmi_set(uint8_t inverted)
{
  glcdSetAddress(MENU_INDENT, 4);
  glcdPutStr("Set ", NORMAL);
  if(cfg_bmi_unit == BMI_Imperial)
  {
  	  glcdPutStr("Imp:", inverted&1);
  	  printnumber_3d(cfg_bmi_weight, inverted&2);
  	  glcdPutStr("lb ", inverted&2);
  	  printnumber_2d(cfg_bmi_height / 12, inverted&4);
  	  glcdPutStr("ft", inverted&4);
  	  printnumber_2d(cfg_bmi_height % 12, inverted&4);
  	  
  }
  else if (cfg_bmi_unit == BMI_Metric)
  {
  	  glcdPutStr("Met:", inverted&1);
  	  glcdWriteChar(' ', NORMAL);
  	  printnumber_3d(cfg_bmi_weight, inverted&2);
  	  glcdPutStr("kg ", inverted&2);
  	  printnumber_3d(cfg_bmi_height, inverted&4);
  	  glcdPutStr("cm", inverted&4);
  }
  else
  {
  	  glcdPutStr("BMI:", inverted&1);
  	  glcdPutStr("         ",NORMAL);
  	  printnumber_3d(cfg_bmi_weight, inverted&2);
  }
}

void display_smoker(uint8_t inverted)
{
	glcdSetAddress(MENU_INDENT, 5);
  glcdPutStr("Smoker?:         ", NORMAL);
  if(cfg_smoker)
  	  glcdPutStr("Yes", inverted);
  else
  	  glcdPutStr(" No", inverted);
}

//Setting the Death Clock needs its own screen.
void display_death_menu(void) {
  cfg_dob_m = eeprom_read_byte(&EE_DOB_MONTH);
  cfg_dob_d = eeprom_read_byte(&EE_DOB_DAY);
  cfg_dob_y = eeprom_read_byte(&EE_DOB_YEAR);
  cfg_gender = eeprom_read_byte(&EE_GENDER);
  cfg_dc_mode = eeprom_read_byte(&EE_DC_MODE);
  cfg_bmi_unit = eeprom_read_byte(&EE_BMI_UNIT);
  cfg_bmi_height = eeprom_read_word(&EE_BMI_HEIGHT);
  cfg_bmi_weight = eeprom_read_word(&EE_BMI_WEIGHT);
  cfg_smoker = eeprom_read_byte(&EE_SMOKER);


  screenmutex++;
  glcdClearScreen();
  
  glcdSetAddress(0, 0);
  glcdPutStr("DeathChron", NORMAL);
  glcdSetAddress(64, 0);	//Not enough space in the 128 bits to put "DeathChron Config Menu"
  glcdPutStr("Config", NORMAL);
  glcdSetAddress(104, 0);  //So these two lines are making up for that. :)
  glcdPutStr("Menu", NORMAL);
  
  //DOB , render based on region setting, in mm/dd/yyyy or dd/mm/yyyy, range is 1900 - 2099.
  display_dob(NORMAL);
  
  //Gender, Male, Female
  display_gender(NORMAL);
  
  //Mode, Normal, Optimistic, Pessimistic, Sadistic
  display_dc_mode(NORMAL);
  
  //BMI Entry Method, Imperial (Weight in pounds, height in X foot Y inches), 
  //Metric (Weight in Kilograms, Height in Centimeters), 
  //Direct (Direct BMI value from 0-255, (actual range for calculation is less then 25, 25-44, and greater then or equal to 45.))
  display_bmi_set(NORMAL);
  
  //Smoking Status.
  display_smoker(NORMAL);
  
  
  
  //Not finished yet.
  /*glcdSetAddress(0, 6);
  glcdPutStr("Not Finished Yet   :)",NORMAL);
  glcdSetAddress(0, 7);
  glcdPutStr("Press Menu to Exit",NORMAL);*/
  menu_advance_set_exit(0);
  
  screenmutex--;
}

void set_deathclock_dob(void) {
   uint8_t mode = SET_DEATHCLOCK_DOB;
   display_death_menu();
   
   screenmutex++;
  // put a small arrow next to 'set date'
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

      if ((mode == SET_DEATHCLOCK_DOB) && (region == REGION_US)) {
	DEBUG(putstring("Set dob month"));
	// ok now its selected
	mode = SET_MONTH;

	// print the month inverted
	display_dob(1);
	// display instructions below
	PLUS_TO_CHANGE(" mon"," set mon.");
      } else if ((mode == SET_DEATHCLOCK_DOB) && (region == REGION_EU)) {
	DEBUG(putstring("Set date month"));
	// ok now its selected
	mode = SET_DAY;

	// print the day inverted
	display_dob(2);
	// display instructions below
	PLUS_TO_CHANGE(" day"," set date");
      } else if ((mode == SET_MONTH) && (region == REGION_US)) {
	DEBUG(putstring("Set date day"));
	mode = SET_DAY;

	// print the month normal
	display_dob(2);
	// display instructions below
	PLUS_TO_CHANGE(" day"," set date");
      }else if ((mode == SET_DAY) && (region == REGION_EU)) {
	DEBUG(putstring("Set date month"));
	mode = SET_MONTH;

	display_dob(1);;
	// display instructions below
	PLUS_TO_CHANGE(" mon"," set mon.");
      } else if ( ((mode == SET_DAY) && (region == REGION_US)) ||
		  ((mode == SET_MONTH) && (region == REGION_EU)) )  {
	DEBUG(putstring("Set year"));
	mode = SET_YEAR;
	// print the date normal

	display_dob(4);
	// display instructions below
	PLUS_TO_CHANGE(" yr."," set year");
      } else {
	// done!
	DEBUG(putstring("done setting date"));
	mode = SET_DEATHCLOCK_DOB;
	// print the seconds normal
	display_dob(NORMAL);
	// display instructions below
	menu_advance_set_exit(0);
	
	//date_y = year;
	//date_m = month;
	//date_d = day;
	eeprom_write_byte(&EE_DOB_MONTH,cfg_dob_m);
    eeprom_write_byte(&EE_DOB_DAY,cfg_dob_d);
    eeprom_write_byte(&EE_DOB_YEAR,cfg_dob_y);
    deathclock_changed();
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;

      screenmutex++;

      if (mode == SET_MONTH) {
	cfg_dob_m++;
	if (cfg_dob_m >= 13)
	  cfg_dob_m = 1;
	if(cfg_dob_m == 2) {
	  if(leapyear(cfg_dob_y + 1900) && (cfg_dob_d > 29))
	  	cfg_dob_d = 29;
	  else if (!leapyear(cfg_dob_y + 1900) && (cfg_dob_d > 28))
	  	cfg_dob_d = 28;
	} else if ((cfg_dob_m == 4) || (cfg_dob_m == 6) || (cfg_dob_m == 9) || (cfg_dob_m == 11)) {
      if(cfg_dob_d > 30)
      	cfg_dob_d = 30;
	}
	display_dob(1);
	
      }
      if (mode == SET_DAY) {
	cfg_dob_d++;
	if (cfg_dob_d > 31)
	  cfg_dob_d = 1;
	if(cfg_dob_m == 2) {
	  if(leapyear(cfg_dob_y + 1900) && (cfg_dob_d > 29))
	  	cfg_dob_d = 1;
	  else if (!leapyear(cfg_dob_y + 1900) && (cfg_dob_d > 28))
	  	cfg_dob_d = 1;
	} else if ((cfg_dob_m == 4) || (cfg_dob_m == 6) || (cfg_dob_m == 9) || (cfg_dob_m == 11)) {
      if(cfg_dob_d > 30)
      	cfg_dob_d = 1;
	}
	display_dob(2);
      }
      if (mode == SET_YEAR) {
	cfg_dob_y = (cfg_dob_y+1) % 200;
	display_dob(4);
      }
      screenmutex--;

      if (pressed & 0x4)
	_delay_ms(200);  
    }
  }
}

void set_deathclock_gender(void) {
  uint8_t mode = SET_DEATHCLOCK_GENDER;

  display_death_menu();
  
  screenmutex++;

  // put a small arrow next to 'set 12h/24h'
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

      if (mode == SET_DEATHCLOCK_GENDER) {
	DEBUG(putstring("Setting deathclock gender"));
	// ok now its selected
	mode = SET_DCGENDER;
	// print the region 
	display_gender(INVERTED);
	// display instructions below
	plus_to_change_default();
      } else {
	mode = SET_DEATHCLOCK_GENDER;
	// print the region normal
	display_gender(NORMAL);

	menu_advance_set_exit(0);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_DCGENDER) {
	    cfg_gender = !cfg_gender;
	screenmutex++;
	//display_death_menu();
	plus_to_change_default();

	// put a small arrow next to 'set 12h/24h'
	drawArrow(0, 19, MENU_INDENT -1);

	display_gender(INVERTED);
	screenmutex--;

	eeprom_write_byte(&EE_GENDER, cfg_gender);
	deathclock_changed();   
      }
    }
  }
}

void set_deathclock_mode(void) {
  uint8_t mode = SET_DEATHCLOCK_MODE;

  display_death_menu();
  
  screenmutex++;

  // put a small arrow next to 'set 12h/24h'
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

      if (mode == SET_DEATHCLOCK_MODE) {
	DEBUG(putstring("Setting deathclock mode"));
	// ok now its selected
	mode = SET_DCMODE;
	// print the region 
	display_dc_mode(INVERTED);
	// display instructions below
	plus_to_change_default();
      } else {
	mode = SET_DEATHCLOCK_MODE;
	// print the region normal
	display_dc_mode(NORMAL);

	menu_advance_set_exit(0);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_DCMODE) {
	    cfg_dc_mode = (cfg_dc_mode + 1) % 4;
	screenmutex++;

	display_dc_mode(INVERTED);
	screenmutex--;

	eeprom_write_byte(&EE_DC_MODE, cfg_dc_mode);
	deathclock_changed();   
      }
    }
  }
}

void set_deathclock_bmi(void) {
  uint8_t mode = SET_DEATHCLOCK_BMI;

  display_death_menu();
  screenmutex++;
  // put a small arrow next to 'set alarm'
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

      if (mode == SET_DEATHCLOCK_BMI) {
	DEBUG(putstring("Set BMI Unit"));
	// ok now its selected
	mode = SET_BMI_UNIT;

	//Set BMI Unit
	display_bmi_set(1);
	// display instructions below
	PLUS_TO_CHANGE(" ut."," set unit");
      } else if (mode == SET_BMI_UNIT) {
	DEBUG(putstring("Set bmi weight / bmi direct"));
	mode = SET_BMI_WT;
	display_bmi_set(2);
	// display instructions below
	if(cfg_bmi_unit != BMI_Direct)
	  PLUS_TO_CHANGE(" wt."," set wt. ");
	else
	  PLUS_TO_CHANGE(" bmi"," set bmi ");
	  } else if ((mode == SET_BMI_WT) && (cfg_bmi_unit != BMI_Direct)) {
	mode = SET_BMI_HT;
    display_bmi_set(4);
    PLUS_TO_CHANGE(" ht."," set ht. ");
      } else {
	mode = SET_DEATHCLOCK_BMI;
	display_bmi_set(NORMAL);
	// display instructions below
	menu_advance_set_exit(0);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_BMI_UNIT) { 
      	  cfg_bmi_unit = (cfg_bmi_unit + 1) % 3;
      	  if(cfg_bmi_unit == BMI_Imperial) {
      	  	  cfg_bmi_weight = 35;
      	  	  cfg_bmi_height = 36;
      	  } else if (cfg_bmi_unit == BMI_Metric) {
      	  	  cfg_bmi_weight = 15;
      	  	  cfg_bmi_height = 92;
      	  } else {
      	  	  cfg_bmi_weight = 0;
      	  }
      	  display_bmi_set(1);
      } 
      if (mode == SET_BMI_WT) {
      	  if(cfg_bmi_unit == BMI_Imperial) {
      	  	  //cfg_bmi_weight = (cfg_bmi_weight + 5) % 660;
      	  	  cfg_bmi_weight += 5;
      	  	  if ( cfg_bmi_weight > 660 )
      	  	  	  cfg_bmi_weight = 35;
      	  } else if (cfg_bmi_unit == BMI_Metric) {
      	  	  cfg_bmi_weight += 5;
      	  	  if ( cfg_bmi_weight > 300 )
      	  	  	  cfg_bmi_weight = 15;
      	  } else {
      	  	  cfg_bmi_weight = (cfg_bmi_weight + 1) % 256;
      	  }
      	  display_bmi_set(2);
      }
      if (mode == SET_BMI_HT) {
      	  if(cfg_bmi_unit == BMI_Imperial) {
      	  	  cfg_bmi_height++;
      	  	  if ( cfg_bmi_height > 120 )
      	  	  	  cfg_bmi_height = 36;
      	  } else if (cfg_bmi_unit == BMI_Metric) {
      	  	  cfg_bmi_height++;
      	  	  if ( cfg_bmi_height > 305 )
      	  	  	  cfg_bmi_height = 92;
      	  }
      	  display_bmi_set(4);
      }
      eeprom_write_byte(&EE_BMI_UNIT,cfg_bmi_unit);
      eeprom_write_word(&EE_BMI_WEIGHT,cfg_bmi_weight);
      eeprom_write_word(&EE_BMI_HEIGHT,cfg_bmi_height);
      deathclock_changed();
      screenmutex--;
      if (pressed & 0x4)
	_delay_ms(200);
    }
  }
}

void set_deathclock_smoker(void) {
  uint8_t mode = SET_DEATHCLOCK_SMOKER;

  display_death_menu();
  menu_advance_set_exit(1);
  
  screenmutex++;

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

      if (mode == SET_DEATHCLOCK_SMOKER) {
	DEBUG(putstring("Setting deathclock smoker status"));
	// ok now its selected
	mode = SET_DCSMOKER;
	// print the region 
	display_smoker(INVERTED);
	// display instructions below
	plus_to_change_default();
      } else {
	mode = SET_DEATHCLOCK_SMOKER;
	// print the region normal
	display_smoker(NORMAL);

	menu_advance_set_exit(1);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_DCSMOKER) {
	    cfg_smoker = !cfg_smoker;
	screenmutex++;

	display_smoker(INVERTED);
	screenmutex--;

	eeprom_write_byte(&EE_SMOKER, cfg_smoker);
	deathclock_changed();   
      }
    }
  }
}

void display_alarm(uint8_t h, uint8_t m)
{
	glcdSetAddress(MENU_INDENT, 1);
  glcdPutStr("Set Alarm:  ", NORMAL);
  print_alarmhour(alarm_h, h);
  glcdWriteChar(':', NORMAL);
  printnumber_2d(alarm_m, m);
}

void display_time(uint8_t h, uint8_t m, uint8_t s, uint8_t inverted)
{
	glcdSetAddress(MENU_INDENT, 2);
  glcdPutStr("Set Time: ", NORMAL);
  print_timehour(h, inverted&1);
  glcdWriteChar(':', NORMAL);
  printnumber_2d(m, inverted&2);
  glcdWriteChar(':', NORMAL);
  printnumber_2d(s, inverted&4);
  if (time_format == TIME_12H) {
    glcdWriteChar(' ', NORMAL);
    if (h >= 12) {
      glcdWriteChar('P', inverted&1);
    } else {
      glcdWriteChar('A', inverted&1);
    }
  }
}

void display_date(uint8_t month, uint8_t day, uint8_t year, uint8_t inverted)
{
  glcdSetAddress(MENU_INDENT, 3);
  glcdPutStr("Set Date:   ", NORMAL);
  if (region == REGION_US) {
    printnumber_2d(month, inverted&1);
    glcdWriteChar('/', NORMAL);
    printnumber_2d(day, inverted&2);
  } else {
    printnumber_2d(day, inverted&2);
    glcdWriteChar('/', NORMAL);
    printnumber_2d(month, inverted&1);
  }
  glcdWriteChar('/', NORMAL);
  printnumber_2d(year, inverted&4);
}

void display_region(uint8_t inverted)
{
  glcdSetAddress(MENU_INDENT, 4);
  glcdPutStr("Set region: ", NORMAL);
  if ((region == REGION_US) && (time_format == TIME_12H)) {
    glcdPutStr("US 12hr", inverted);
  } else if ((region == REGION_US) && (time_format == TIME_24H)) {
    glcdPutStr("US 24hr", inverted);
  } else if ((region == REGION_EU) && (time_format == TIME_12H)) {
    glcdPutStr("EU 12hr", inverted);
  } else {
    glcdPutStr("EU 24hr", inverted);
  }
}

#ifdef BACKLIGHT_ADJUST
void display_backlight(uint8_t inverted)
{
  glcdSetAddress(MENU_INDENT, 5);
  glcdPutStr("Set Backlight: ", NORMAL);
  printnumber_2d(OCR2B>>OCR2B_BITSHIFT,inverted);
}
#endif

void display_menu(void) {
  DEBUGP("display menu");
  
  screenmutex++;

  glcdClearScreen();
  
  glcdSetAddress(0, 0);
  glcdPutStr("Configuration Menu", NORMAL);
  display_alarm(NORMAL,NORMAL);
  display_time(time_h,time_m,time_s,NORMAL);
  display_date(date_m,date_d,date_y,NORMAL);
  display_region(NORMAL);
#ifdef BACKLIGHT_ADJUST
  display_backlight(NORMAL);
#endif
  
  menu_advance_set_exit(0);

  screenmutex--;
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

      if ((mode == SET_DATE) && (region == REGION_US)) {
	DEBUG(putstring("Set date month"));
	// ok now its selected
	mode = SET_MONTH;

	// print the month inverted
	display_date(month,day,year,1);
	// display instructions below
	PLUS_TO_CHANGE(" mon"," set mon.");
      } else if ((mode == SET_DATE) && (region == REGION_EU)) {
	DEBUG(putstring("Set date month"));
	// ok now its selected
	mode = SET_DAY;

	// print the day inverted
	display_date(month,day,year,2);
	// display instructions below
	PLUS_TO_CHANGE(" day"," set date");
      } else if ((mode == SET_MONTH) && (region == REGION_US)) {
	DEBUG(putstring("Set date day"));
	mode = SET_DAY;

	display_date(month,day,year,2);
	// display instructions below
	PLUS_TO_CHANGE(" day"," set date");
      }else if ((mode == SET_DAY) && (region == REGION_EU)) {
	DEBUG(putstring("Set date month"));
	mode = SET_MONTH;

	display_date(month,day,year,1);
	// display instructions below
	PLUS_TO_CHANGE(" mon"," set mon.");
      } else if ( ((mode == SET_DAY) && (region == REGION_US)) ||
		  ((mode == SET_MONTH) && (region == REGION_EU)) )  {
	DEBUG(putstring("Set year"));
	mode = SET_YEAR;
	// print the date normal

	display_date(month,day,year,4);
	// display instructions below
	PLUS_TO_CHANGE(" yr."," set year");
      } else {
	// done!
	DEBUG(putstring("done setting date"));
	mode = SET_DATE;
	// print the seconds normal
	display_date(month,day,year,NORMAL);
	// display instructions below
	menu_advance_set_exit(0);
	
	date_y = year;
	date_m = month;
	date_d = day;
	writei2ctime(time_s, time_m, time_h, 0, date_d, date_m, date_y);
      }
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
	display_date(month,day,year,1);
	
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
	display_date(month,day,year,2);
      }
      if (mode == SET_YEAR) {
	year = (year+1) % 100;
	display_date(month,day,year,4);
      }
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
  //glcdSetAddress(0, 6);
  //glcdPutStr("Press MENU to exit   ", NORMAL);
  

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
    
    if(mode == SET_BRIGHTNESS)
    {
    	screenmutex++;
    	glcdSetAddress(0, 6);
    	if((time_s%4)<2)
        	glcdPutStr("Press MENU to advance", NORMAL);
        else
        	glcdPutStr("to the next page.    ", NORMAL);
    	screenmutex--;
    }
  
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_BRIGHTNESS) {
	DEBUG(putstring("Setting backlight"));
	// ok now its selected
	mode = SET_BRT;
	// print the region 
	display_backlight(INVERTED);
	
	// display instructions below
	plus_to_change_default();
      } else {
	mode = SET_BRIGHTNESS;
	// print the region normal
	display_backlight(NORMAL);

	menu_advance_set_exit(0);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_BRT) {
	    OCR2B += OCR2B_PLUS;
	    if(OCR2B > OCR2A_VALUE)
	      OCR2B = 0;
	screenmutex++;
	display_backlight(INVERTED);
	screenmutex--;

	eeprom_write_byte(&EE_BRIGHT, OCR2B);
      }
    }
  }
}
#endif


void set_region(void) {
  uint8_t mode = SET_REGION;

  display_menu();
  
  screenmutex++;

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
	display_region(INVERTED);
	// display instructions below
	plus_to_change_default();
      } else {
	mode = SET_REGION;
	// print the region normal
	display_region(NORMAL);

	menu_advance_set_exit(0);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_REG) {
	    if(time_format) {        
	      region = !region;
		  time_format = !time_format;
		} else {
		  time_format = !time_format;
		}
	screenmutex++;
	display_menu();
	plus_to_change_default();

	// put a small arrow next to 'set 12h/24h'
	drawArrow(0, 35, MENU_INDENT -1);

	display_region(INVERTED);
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

	display_alarm(INVERTED,NORMAL);
	// display instructions below
	PLUS_TO_CHANGE(" hr."," set hour");
      } else if (mode == SET_HOUR) {
	DEBUG(putstring("Set alarm min"));
	mode = SET_MIN;
	// print the hour normal
	display_alarm(NORMAL,INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" min"," set mins");

      } else {
	mode = SET_ALARM;
	// print the hour normal
	display_alarm(NORMAL,NORMAL);
	// display instructions below
	menu_advance_set_exit(0);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_HOUR) {
	alarm_h = (alarm_h+1) % 24;
	// print the hour inverted
	display_alarm(INVERTED,NORMAL);
	eeprom_write_byte(&EE_ALARM_HOUR, alarm_h);    
      }
      if (mode == SET_MIN) {
	alarm_m = (alarm_m+1) % 60;
	display_alarm(NORMAL,INVERTED);
	eeprom_write_byte(&EE_ALARM_MIN, alarm_m);    
      }
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

	display_time(hour,min,sec,1);

	// display instructions below
	PLUS_TO_CHANGE(" hr."," set hour");
      } else if (mode == SET_HOUR) {
	DEBUG(putstring("Set time min"));
	mode = SET_MIN;
	// print the hour normal
	display_time(hour,min,sec,2);
	PLUS_TO_CHANGE(" min"," set mins");
      } else if (mode == SET_MIN) {
	DEBUG(putstring("Set time sec"));
	mode = SET_SEC;
	// and the minutes normal
	display_time(hour,min,sec,4);
	// display instructions below
	PLUS_TO_CHANGE(" sec"," set secs");
      } else {
	// done!
	DEBUG(putstring("done setting time"));
	mode = SET_TIME;
	// print the seconds normal
	display_time(hour,min,sec,NORMAL);
	// display instructions below
	menu_advance_set_exit(0);
	
	time_h = hour;
	time_m = min;
	time_s = sec;
	writei2ctime(time_s, time_m, time_h, 0, date_d, date_m, date_y);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      screenmutex++;
      if (mode == SET_HOUR) {
	hour = (hour+1) % 24;
	time_h = hour;
	
	display_time(hour,min,sec,1);
      }
      if (mode == SET_MIN) {
	min = (min+1) % 60;
	display_time(hour,min,sec,2);
      }
      if (mode == SET_SEC) {
	sec = (sec+1) % 60;
	display_time(hour,min,sec,4);
      }
      screenmutex--;
      if (pressed & 0x4)
	_delay_ms(200);
    }
  }
}

void print_timehour(uint8_t h, uint8_t inverted) {
  if (time_format == TIME_12H) {
    if (((h + 23)%12 + 1) >= 10 ) {
      printnumber_2d((h + 23)%12 + 1, inverted);
    } else {
      glcdWriteChar(' ', NORMAL);
      glcdWriteChar('0' + (h + 23)%12 + 1, inverted);
    }
  } else {
    glcdWriteChar(' ', NORMAL);
    glcdWriteChar(' ', NORMAL);
    printnumber_2d(h, inverted);
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
      printnumber_2d((h + 23)%12 + 1, inverted);
    } else {
      glcdWriteChar(' ', NORMAL);
      glcdWriteChar('0' + (h + 23)%12 + 1, inverted);
    }
   } else {
    glcdSetAddress(MENU_INDENT + 12*6, 1);
    printnumber_2d(h, inverted);
  }
}
