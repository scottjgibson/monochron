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

#ifdef DEATHCHRON

extern volatile uint8_t displaystyle;
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
extern volatile uint8_t timeoutcounter;

extern volatile uint8_t screenmutex;

void printnumber_3d(uint16_t n, uint8_t inverted) {
  glcdWriteChar(n/100+'0', inverted);
  glcdWriteChar((n%100)/10+'0', inverted);
  glcdWriteChar(n%10+'0', inverted);
}

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
    printnumber(cfg_dob_m, inverted&1);
    glcdWriteChar('/', NORMAL);
    printnumber(cfg_dob_d, inverted&2);
  } else {
    printnumber(cfg_dob_d, inverted&2);
    glcdWriteChar('/', NORMAL);
    printnumber(cfg_dob_m, inverted&1);
  }
  glcdWriteChar('/', NORMAL);
  printnumber((cfg_dob_y+1900)/100, inverted&4);
  printnumber((cfg_dob_y+1900)%100, inverted&4);
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
  	  printnumber(cfg_bmi_height / 12, inverted&4);
  	  glcdPutStr("ft", inverted&4);
  	  printnumber(cfg_bmi_height % 12, inverted&4);
  	  
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
	delay_ms(200);  
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
	delay_ms(200);
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

void initanim_deathcfg(void) {

  load_etd();	//Only need to do this once at power on, and once if Death Clock settings are changed, and refresh if date/time is changed.
  just_pressed = 1;
  do {
    if (just_pressed & 0x1) {
      just_pressed = 0;
      switch(displaymode) {
      case (SHOW_TIME):
	displaymode = SET_DEATHCLOCK_DOB;
	set_deathclock_dob();
	break;
	  case SET_DEATHCLOCK_DOB:
	displaymode = SET_DEATHCLOCK_GENDER;
	set_deathclock_gender();
	break;
	  case SET_DEATHCLOCK_GENDER:
	displaymode = SET_DEATHCLOCK_MODE;
	set_deathclock_mode();
	break;
	  case SET_DEATHCLOCK_MODE:
	displaymode = SET_DEATHCLOCK_BMI;
	set_deathclock_bmi();
	break;
	  case SET_DEATHCLOCK_BMI:
	displaymode = SET_DEATHCLOCK_SMOKER;
	set_deathclock_smoker();
	break;
      default:
    just_pressed = 1;	//Exit, returning to the main cfg menu.
	displaymode = CFG_MENU;
	displaystyle=eeprom_read_byte(&EE_STYLE);
	glcdClearScreen();
	return;
      }
    }
  } while (displaymode != SHOW_TIME);
  
}

#endif