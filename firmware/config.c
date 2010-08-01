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

volatile uint8_t cfg_dob_d, cfg_dob_m, cfg_dob_y, cfg_gender, cfg_dc_mode, cfg_bmi_unit, cfg_smoker;
volatile uint16_t cfg_bmi_height, cfg_bmi_weight;

extern volatile uint8_t displaymode;
// This variable keeps track of whether we have not pressed any
// buttons in a few seconds, and turns off the menu display
volatile uint8_t timeoutcounter = 0;

volatile uint8_t screenmutex = 0;

void deathclock_changed(void) //Any changes to the death clock neccesitates a recalculation of the death clock.
{
	eeprom_write_byte((uint8_t *)EE_SET_MONTH,date_m);
	eeprom_write_byte((uint8_t *)EE_SET_DAY,date_d);
	eeprom_write_byte((uint8_t *)EE_SET_YEAR,date_y);
	eeprom_write_byte((uint8_t *)EE_SET_HOUR,time_h);
	eeprom_write_byte((uint8_t *)EE_SET_MIN,time_m);
	eeprom_write_byte((uint8_t *)EE_SET_SEC,time_s);
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

void plus_to_change_default(void)
{
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change    ", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to save    ", NORMAL);
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

//Setting the Death Clock needs its own screen.
void display_death_menu(void) {
  cfg_dob_m = eeprom_read_byte((uint8_t *)EE_DOB_MONTH);
  cfg_dob_d = eeprom_read_byte((uint8_t *)EE_DOB_DAY);
  cfg_dob_y = eeprom_read_byte((uint8_t *)EE_DOB_YEAR);
  cfg_gender = eeprom_read_byte((uint8_t *)EE_GENDER);
  cfg_dc_mode = eeprom_read_byte((uint8_t *)EE_DC_MODE);
  cfg_bmi_unit = eeprom_read_byte((uint8_t *)EE_BMI_UNIT);
  cfg_bmi_height = eeprom_read_word((uint16_t *)EE_BMI_HEIGHT);
  cfg_bmi_weight = eeprom_read_word((uint16_t *)EE_BMI_WEIGHT);
  cfg_smoker = eeprom_read_byte((uint8_t *)EE_SMOKER);


  screenmutex++;
  glcdClearScreen();
  
  glcdSetAddress(0, 0);
  glcdPutStr("Death Clock Menu", NORMAL);
  
  //DOB , render based on region setting, in mm/dd/yyyy or dd/mm/yyyy, range is 1900 - 2099.
  glcdSetAddress(MENU_INDENT, 1);
  glcdPutStr("Set DOB:  ",NORMAL);
   if (region == REGION_US) {
    printnumber_2d(cfg_dob_m, NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber_2d(cfg_dob_d, NORMAL);
  } else {
    printnumber_2d(cfg_dob_d, NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber_2d(cfg_dob_m, NORMAL);
  }
  glcdWriteChar('/', NORMAL);
  printnumber_2d((cfg_dob_y+1900)/100, NORMAL);
  printnumber_2d((cfg_dob_y+1900)%100, NORMAL);
 
  //Gender, Male, Female
  glcdSetAddress(MENU_INDENT, 2);
  glcdPutStr("Set Gender:   ",NORMAL);
  if(cfg_gender==DC_gender_male)
  	  glcdPutStr("  Male", NORMAL);
  else
  	  glcdPutStr("Female", NORMAL);
  
  //Mode, Normal, Optimistic, Pessimistic, Sadistic
  glcdSetAddress(MENU_INDENT, 3);
  glcdPutStr("Set Mode:", NORMAL);
  if(cfg_dc_mode == DC_mode_normal)
    glcdPutStr("     Normal", NORMAL);
  else if (cfg_dc_mode == DC_mode_pessimistic)
    glcdPutStr("Pessimistic", NORMAL);
  else if (cfg_dc_mode == DC_mode_optimistic)
    glcdPutStr(" Optimistic", NORMAL);
  else
    glcdPutStr("   Sadistic", NORMAL);
  
  //BMI Entry Method, Imperial (Weight in pounds, height in X foot Y inches), 
  //Metric (Weight in Kilograms, Height in Centimeters), 
  //Direct (Direct BMI value from 0-255, (actual range for calculation is less then 25, 25-44, and greater then or equal to 45.))
  glcdSetAddress(MENU_INDENT, 4);
  glcdPutStr("Set ", NORMAL);
  if(cfg_bmi_unit == BMI_Imperial)
  {
  	  glcdPutStr("Imp:", NORMAL);
  	  printnumber_3d(cfg_bmi_weight, NORMAL);
  	  glcdPutStr("lb ", NORMAL);
  	  printnumber_2d(cfg_bmi_height / 12, NORMAL);
  	  glcdPutStr("ft", NORMAL);
  	  printnumber_2d(cfg_bmi_height % 12, NORMAL);
  	  
  }
  else if (cfg_bmi_unit == BMI_Metric)
  {
  	  glcdPutStr("Met: ", NORMAL);
  	  printnumber_3d(cfg_bmi_weight, NORMAL);
  	  glcdPutStr("kg ", NORMAL);
  	  printnumber_3d(cfg_bmi_height, NORMAL);
  	  glcdPutStr("cm", NORMAL);
  }
  else
  {
  	  glcdPutStr("BMI:         ", NORMAL);
  	  printnumber_3d(cfg_bmi_weight, NORMAL);
  }
  
  //Smoking Status.
  glcdSetAddress(MENU_INDENT, 5);
  glcdPutStr("Smoker?:         ", NORMAL);
  if(cfg_smoker)
  	  glcdPutStr("Yes", NORMAL);
  else
  	  glcdPutStr(" No", NORMAL);
  
  
  
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
	glcdSetAddress(MENU_INDENT + 10*6, 1);
	printnumber_2d(cfg_dob_m, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" mon"," set mon.");
      } else if ((mode == SET_DEATHCLOCK_DOB) && (region == REGION_EU)) {
	DEBUG(putstring("Set date month"));
	// ok now its selected
	mode = SET_DAY;

	// print the day inverted
	glcdSetAddress(MENU_INDENT + 10*6, 1);
	printnumber_2d(cfg_dob_d, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" day"," set date");
      } else if ((mode == SET_MONTH) && (region == REGION_US)) {
	DEBUG(putstring("Set date day"));
	mode = SET_DAY;

	// print the month normal
	glcdSetAddress(MENU_INDENT + 10*6, 1);
	printnumber_2d(cfg_dob_m, NORMAL);
	// and the day inverted
	glcdWriteChar('/', NORMAL);
	printnumber_2d(cfg_dob_d, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" day"," set date");
      }else if ((mode == SET_DAY) && (region == REGION_EU)) {
	DEBUG(putstring("Set date month"));
	mode = SET_MONTH;

	// print the day normal
	glcdSetAddress(MENU_INDENT + 10*6, 1);
	printnumber_2d(cfg_dob_d, NORMAL);
	// and the month inverted
	glcdWriteChar('/', NORMAL);
	printnumber_2d(cfg_dob_m, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" mon"," set mon.");
      } else if ( ((mode == SET_DAY) && (region == REGION_US)) ||
		  ((mode == SET_MONTH) && (region == REGION_EU)) )  {
	DEBUG(putstring("Set year"));
	mode = SET_YEAR;
	// print the date normal

	glcdSetAddress(MENU_INDENT + 10*6, 1);
	if (region == REGION_US) {
	  printnumber_2d(cfg_dob_m, NORMAL);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(cfg_dob_d, NORMAL);
	} else {
	  printnumber_2d(cfg_dob_d, NORMAL);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(cfg_dob_m, NORMAL);
	}
	glcdWriteChar('/', NORMAL);
	// and the year inverted
	printnumber_2d((cfg_dob_y/100)+19, INVERTED);
	printnumber_2d(cfg_dob_y%100, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" yr."," set year");
      } else {
	// done!
	DEBUG(putstring("done setting date"));
	mode = SET_DEATHCLOCK_DOB;
	// print the seconds normal
	glcdSetAddress(MENU_INDENT + 16*6, 1);
	printnumber_2d((cfg_dob_y/100)+19, NORMAL);
	printnumber_2d(cfg_dob_y%100, NORMAL);
	// display instructions below
	menu_advance_set_exit(0);
	
	//date_y = year;
	//date_m = month;
	//date_d = day;
	eeprom_write_byte((uint8_t *)EE_DOB_MONTH,cfg_dob_m);
    eeprom_write_byte((uint8_t *)EE_DOB_DAY,cfg_dob_d);
    eeprom_write_byte((uint8_t *)EE_DOB_YEAR,cfg_dob_y);
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
	glcdSetAddress(MENU_INDENT + 10*6, 1);
	if (region == REGION_US) {
	  printnumber_2d(cfg_dob_m, INVERTED);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(cfg_dob_d, NORMAL);
	} else {
	  printnumber_2d(cfg_dob_d, NORMAL);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(cfg_dob_m, INVERTED);
	}
	
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
	glcdSetAddress(MENU_INDENT + 10*6, 1);
	if (region == REGION_US) {
	  printnumber_2d(cfg_dob_m, NORMAL);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(cfg_dob_d, INVERTED);
	} else {
	  printnumber_2d(cfg_dob_d, INVERTED);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(cfg_dob_m, NORMAL);
	}
      }
      if (mode == SET_YEAR) {
	cfg_dob_y = (cfg_dob_y+1) % 200;
	glcdSetAddress(MENU_INDENT + 16*6, 1);
	printnumber_2d((cfg_dob_y/100)+19, INVERTED);
	printnumber_2d(cfg_dob_y%100, INVERTED);
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
	glcdSetAddress(MENU_INDENT + 14*6, 2);
	if(cfg_gender == DC_gender_male)
	  glcdPutStr("  Male", INVERTED);
	else
	  glcdPutStr("Female", INVERTED);
	// display instructions below
	plus_to_change_default();
      } else {
	mode = SET_DEATHCLOCK_GENDER;
	// print the region normal
	glcdSetAddress(MENU_INDENT + 14*6, 2);
	if(cfg_gender == DC_gender_male)
	  glcdPutStr("  Male", NORMAL);
	else
	  glcdPutStr("Female", NORMAL);

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

	glcdSetAddress(MENU_INDENT + 14*6, 2);
	if(cfg_gender == DC_gender_male)
	  glcdPutStr("  Male", INVERTED);
	else
	  glcdPutStr("Female", INVERTED);
	screenmutex--;

	eeprom_write_byte((uint8_t *)EE_GENDER, cfg_gender);
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
	glcdSetAddress(MENU_INDENT + 9*6, 3);
	if(cfg_dc_mode == DC_mode_normal)
	  glcdPutStr("     Normal", INVERTED);
	else if (cfg_dc_mode == DC_mode_pessimistic)
	  glcdPutStr("Pessimistic", INVERTED);
	else if (cfg_dc_mode == DC_mode_sadistic)
	  glcdPutStr("   Sadistic", INVERTED);
	else
	  glcdPutStr(" Optimistic", INVERTED);
	// display instructions below
	plus_to_change_default();
      } else {
	mode = SET_DEATHCLOCK_MODE;
	// print the region normal
	glcdSetAddress(MENU_INDENT + 9*6, 3);
	if(cfg_dc_mode == DC_mode_normal)
	  glcdPutStr("     Normal", NORMAL);
	else if (cfg_dc_mode == DC_mode_pessimistic)
	  glcdPutStr("Pessimistic", NORMAL);
	else if (cfg_dc_mode == DC_mode_sadistic)
	  glcdPutStr("   Sadistic", NORMAL);
	else
	  glcdPutStr(" Optimistic", NORMAL);

	menu_advance_set_exit(0);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_DCMODE) {
	    cfg_dc_mode = (cfg_dc_mode + 1) % 4;
	screenmutex++;

	glcdSetAddress(MENU_INDENT + 9*6, 3);
	if(cfg_dc_mode == DC_mode_normal)
	  glcdPutStr("     Normal", INVERTED);
	else if (cfg_dc_mode == DC_mode_pessimistic)
	  glcdPutStr("Pessimistic", INVERTED);
	else if (cfg_dc_mode == DC_mode_sadistic)
	  glcdPutStr("   Sadistic", INVERTED);
	else
	  glcdPutStr(" Optimistic", INVERTED);
	screenmutex--;

	eeprom_write_byte((uint8_t *)EE_DC_MODE, cfg_dc_mode);
	deathclock_changed();   
      }
    }
  }
}

void display_menu(void) {
  DEBUGP("display menu");
  
  screenmutex++;

  glcdClearScreen();
  
  glcdSetAddress(0, 0);
  glcdPutStr("Configuration Menu", NORMAL);
  
  glcdSetAddress(MENU_INDENT, 1);
  glcdPutStr("Set Alarm:  ", NORMAL);
  print_alarmhour(alarm_h, NORMAL);
  glcdWriteChar(':', NORMAL);
  printnumber_2d(alarm_m, NORMAL);
  
  glcdSetAddress(MENU_INDENT, 2);
  glcdPutStr("Set Time: ", NORMAL);
  print_timehour(time_h, NORMAL);
  glcdWriteChar(':', NORMAL);
  printnumber_2d(time_m, NORMAL);
  glcdWriteChar(':', NORMAL);
  printnumber_2d(time_s, NORMAL);
  if (time_format == TIME_12H) {
    glcdWriteChar(' ', NORMAL);
    if (time_h >= 12) {
      glcdWriteChar('P', NORMAL);
    } else {
      glcdWriteChar('A', NORMAL);
    }
  }
  
  glcdSetAddress(MENU_INDENT, 3);
  glcdPutStr("Set Date:   ", NORMAL);
  if (region == REGION_US) {
    printnumber_2d(date_m, NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber_2d(date_d, NORMAL);
  } else {
    printnumber_2d(date_d, NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber_2d(date_m, NORMAL);
  }
  glcdWriteChar('/', NORMAL);
  printnumber_2d(date_y, NORMAL);
  
  glcdSetAddress(MENU_INDENT, 4);
  glcdPutStr("Set region: ", NORMAL);
  if ((region == REGION_US) && (time_format == TIME_12H)) {
    glcdPutStr("US 12hr", NORMAL);
  } else if ((region == REGION_US) && (time_format == TIME_24H)) {
    glcdPutStr("US 24hr", NORMAL);
  } else if ((region == REGION_EU) && (time_format == TIME_12H)) {
    glcdPutStr("EU 12hr", NORMAL);
  } else {
    glcdPutStr("EU 24hr", NORMAL);
  }
  
#ifdef BACKLIGHT_ADJUST
  glcdSetAddress(MENU_INDENT, 5);
  glcdPutStr("Set Backlight: ", NORMAL);
  printnumber_2d(OCR2B>>OCR2B_BITSHIFT,NORMAL);
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
	glcdSetAddress(MENU_INDENT + 12*6, 3);
	printnumber_2d(month, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" mon"," set mon.");
      } else if ((mode == SET_DATE) && (region == REGION_EU)) {
	DEBUG(putstring("Set date month"));
	// ok now its selected
	mode = SET_DAY;

	// print the day inverted
	glcdSetAddress(MENU_INDENT + 12*6, 3);
	printnumber_2d(day, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" day"," set date");
      } else if ((mode == SET_MONTH) && (region == REGION_US)) {
	DEBUG(putstring("Set date day"));
	mode = SET_DAY;

	// print the month normal
	glcdSetAddress(MENU_INDENT + 12*6, 3);
	printnumber_2d(month, NORMAL);
	// and the day inverted
	glcdWriteChar('/', NORMAL);
	printnumber_2d(day, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" day"," set date");
      }else if ((mode == SET_DAY) && (region == REGION_EU)) {
	DEBUG(putstring("Set date month"));
	mode = SET_MONTH;

	// print the day normal
	glcdSetAddress(MENU_INDENT + 12*6, 3);
	printnumber_2d(day, NORMAL);
	// and the month inverted
	glcdWriteChar('/', NORMAL);
	printnumber_2d(month, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" mon"," set mon.");
      } else if ( ((mode == SET_DAY) && (region == REGION_US)) ||
		  ((mode == SET_MONTH) && (region == REGION_EU)) )  {
	DEBUG(putstring("Set year"));
	mode = SET_YEAR;
	// print the date normal

	glcdSetAddress(MENU_INDENT + 12*6, 3);
	if (region == REGION_US) {
	  printnumber_2d(month, NORMAL);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(day, NORMAL);
	} else {
	  printnumber_2d(day, NORMAL);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(month, NORMAL);
	}
	glcdWriteChar('/', NORMAL);
	// and the year inverted
	printnumber_2d(year, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" yr."," set year");
      } else {
	// done!
	DEBUG(putstring("done setting date"));
	mode = SET_DATE;
	// print the seconds normal
	glcdSetAddress(MENU_INDENT + 18*6, 3);
	printnumber_2d(year, NORMAL);
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
	glcdSetAddress(MENU_INDENT + 12*6, 3);
	if (region == REGION_US) {
	  printnumber_2d(month, INVERTED);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(day, NORMAL);
	} else {
	  printnumber_2d(day, NORMAL);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(month, INVERTED);
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
	glcdSetAddress(MENU_INDENT + 12*6, 3);
	if (region == REGION_US) {
	  printnumber_2d(month, NORMAL);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(day, INVERTED);
	} else {
	  printnumber_2d(day, INVERTED);
	  glcdWriteChar('/', NORMAL);
	  printnumber_2d(month, NORMAL);
	}
      }
      if (mode == SET_YEAR) {
	year = (year+1) % 100;
	glcdSetAddress(MENU_INDENT + 18*6, 3);
	printnumber_2d(year, INVERTED);
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
  
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_BRIGHTNESS) {
	DEBUG(putstring("Setting backlight"));
	// ok now its selected
	mode = SET_BRT;
	// print the region 
	glcdSetAddress(MENU_INDENT + 15*6, 5);
	printnumber_2d(OCR2B>>OCR2B_BITSHIFT,INVERTED);
	
	// display instructions below
	plus_to_change_default();
      } else {
	mode = SET_BRIGHTNESS;
	// print the region normal
	glcdSetAddress(MENU_INDENT + 15*6, 5);
	printnumber_2d(OCR2B>>OCR2B_BITSHIFT,NORMAL);

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
	display_menu();
	plus_to_change_default();

	// put a small arrow next to 'set 12h/24h'
	drawArrow(0, 43, MENU_INDENT -1);
	glcdSetAddress(MENU_INDENT + 15*6, 5);
	printnumber_2d(OCR2B>>OCR2B_BITSHIFT,INVERTED);
	
	screenmutex--;

	eeprom_write_byte((uint8_t *)EE_BRIGHT, OCR2B);
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
	glcdSetAddress(MENU_INDENT + 12*6, 4);
	if ((region == REGION_US) && (time_format == TIME_12H)) {
	  glcdPutStr("US 12hr", INVERTED);
	} else if ((region == REGION_US) && (time_format == TIME_24H)) {
	  glcdPutStr("US 24hr", INVERTED);
	} else if ((region == REGION_EU) && (time_format == TIME_12H)) {
	  glcdPutStr("EU 12hr", INVERTED);
	} else {
	  glcdPutStr("EU 24hr", INVERTED);
	}
	// display instructions below
	plus_to_change_default();
      } else {
	mode = SET_REGION;
	// print the region normal
	glcdSetAddress(MENU_INDENT + 12*6, 4);
	if ((region == REGION_US) && (time_format == TIME_12H)) {
	  glcdPutStr("US 12hr", NORMAL);
	} else if ((region == REGION_US) && (time_format == TIME_24H)) {
	  glcdPutStr("US 24hr", NORMAL);
	} else if ((region == REGION_EU) && (time_format == TIME_12H)) {
	  glcdPutStr("EU 12hr", NORMAL);
	} else {
	  glcdPutStr("EU 24hr", NORMAL);
	}

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

	glcdSetAddress(MENU_INDENT + 12*6, 4);
	if ((region == REGION_US) && (time_format == TIME_12H)) {
	  glcdPutStr("US 12hr", INVERTED);
	} else if ((region == REGION_US) && (time_format == TIME_24H)) {
	  glcdPutStr("US 24hr", INVERTED);
	} else if ((region == REGION_EU) && (time_format == TIME_12H)) {
	  glcdPutStr("EU 12hr", INVERTED);
	} else {
	  glcdPutStr("EU 24hr", INVERTED);
	}
	screenmutex--;

	eeprom_write_byte((uint8_t *)EE_REGION, region);
	eeprom_write_byte((uint8_t *)EE_TIME_FORMAT, time_format);    
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

	// print the hour inverted
	print_alarmhour(alarm_h, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" hr."," set hour");
      } else if (mode == SET_HOUR) {
	DEBUG(putstring("Set alarm min"));
	mode = SET_MIN;
	// print the hour normal
	glcdSetAddress(MENU_INDENT + 12*6, 1);
	print_alarmhour(alarm_h, NORMAL);
	// and the minutes inverted
	glcdSetAddress(MENU_INDENT + 15*6, 1);
	printnumber_2d(alarm_m, INVERTED);
	// display instructions below
	PLUS_TO_CHANGE(" min"," set mins");

      } else {
	mode = SET_ALARM;
	// print the hour normal
	glcdSetAddress(MENU_INDENT + 12*6, 1);
	print_alarmhour(alarm_h, NORMAL);
	// and the minutes inverted
	glcdSetAddress(MENU_INDENT + 15*6, 1);
	printnumber_2d(alarm_m, NORMAL);
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
	print_alarmhour(alarm_h, INVERTED);
	eeprom_write_byte((uint8_t *)EE_ALARM_HOUR, alarm_h);    
      }
      if (mode == SET_MIN) {
	alarm_m = (alarm_m+1) % 60;
	glcdSetAddress(MENU_INDENT + 15*6, 1);
	printnumber_2d(alarm_m, INVERTED);
	eeprom_write_byte((uint8_t *)EE_ALARM_MIN, alarm_m);    
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

	// print the hour inverted
	glcdSetAddress(MENU_INDENT + 10*6, 2);
	print_timehour(hour, INVERTED);
	glcdSetAddress(MENU_INDENT + 18*6, 2);
	if (time_format == TIME_12H) {
	  glcdWriteChar(' ', NORMAL);
	  if (hour >= 12) {
	    glcdWriteChar('P', INVERTED);
	  } else {
	    glcdWriteChar('A', INVERTED);
	  }
	}

	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change hr.", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set hour", NORMAL);
      } else if (mode == SET_HOUR) {
	DEBUG(putstring("Set time min"));
	mode = SET_MIN;
	// print the hour normal
	glcdSetAddress(MENU_INDENT + 10*6, 2);
	print_timehour(hour, NORMAL);
	// and the minutes inverted
	glcdWriteChar(':', NORMAL);
	printnumber_2d(min, INVERTED);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change min", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set mins", NORMAL);

	glcdSetAddress(MENU_INDENT + 18*6, 2);
	if (time_format == TIME_12H) {
	  glcdWriteChar(' ', NORMAL);
	  if (hour >= 12) {
	    glcdWriteChar('P', NORMAL);
	  } else {
	    glcdWriteChar('A', NORMAL);
	  }
	}
      } else if (mode == SET_MIN) {
	DEBUG(putstring("Set time sec"));
	mode = SET_SEC;
	// and the minutes normal
	if(time_format == TIME_12H) {
	  glcdSetAddress(MENU_INDENT + 13*6, 2);
	} else {
	  glcdSetAddress(MENU_INDENT + 15*6, 2);
	}
	printnumber_2d(min, NORMAL);
	glcdWriteChar(':', NORMAL);
	// and the seconds inverted
	printnumber_2d(sec, INVERTED);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change sec", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set secs", NORMAL);
      } else {
	// done!
	DEBUG(putstring("done setting time"));
	mode = SET_TIME;
	// print the seconds normal
	if(time_format == TIME_12H) {
	  glcdSetAddress(MENU_INDENT + 16*6, 2);
	} else {
  	  glcdSetAddress(MENU_INDENT + 18*6, 2);
	}
	printnumber_2d(sec, NORMAL);
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
	
	glcdSetAddress(MENU_INDENT + 10*6, 2);
	print_timehour(hour, INVERTED);
	glcdSetAddress(MENU_INDENT + 18*6, 2);
	if (time_format == TIME_12H) {
	  glcdWriteChar(' ', NORMAL);
	  if (time_h >= 12) {
	    glcdWriteChar('P', INVERTED);
	  } else {
	    glcdWriteChar('A', INVERTED);
	  }
	}
      }
      if (mode == SET_MIN) {
	min = (min+1) % 60;
	if(time_format == TIME_12H) {
	  glcdSetAddress(MENU_INDENT + 13*6, 2);
	} else {
	  glcdSetAddress(MENU_INDENT + 15*6, 2);
	}
	printnumber_2d(min, INVERTED);
      }
      if (mode == SET_SEC) {
	sec = (sec+1) % 60;
	if(time_format == TIME_12H) {
	  glcdSetAddress(MENU_INDENT + 16*6, 2);
	} else {
	  glcdSetAddress(MENU_INDENT + 18*6, 2);
	}
	printnumber_2d(sec, INVERTED);
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
