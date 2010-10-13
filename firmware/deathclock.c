/***************************************************************************
 MonoChron Death Clock firmware July 28, 2010
 (c) 2010 Damien Good

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include <avr/io.h>      
#include <string.h>
#include <avr/interrupt.h>   // Interrupts and timers
#include <util/delay.h>      // Blocking delay functions
#include <avr/pgmspace.h>    // So we can store the 'font table' in ROM
#include <avr/eeprom.h>      // Date/time/pref backup in permanent EEPROM
#include <avr/wdt.h>     // Watchdog timer to repair lockups

#include "deathclock.h"
#include "ratt.h"
#include "util.h"


#ifdef DEATHCHRON
extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t date_m, date_d, date_y;
volatile uint8_t death_m, death_d, death_y;

volatile int32_t minutes_left=0;
volatile int32_t old_minutes_left;
volatile uint8_t dc_mode;
volatile uint8_t reaper_tow_rip;

const uint8_t bmi_table_offset[] = {
	0,5,	//normal
	0,5,	//pessimistic
	0,5,	//optimistic
	11,12,	//Sadistic
};

const uint8_t bmi_table_count[] = {
	5,6,	//normal
	5,6,	//pessimistic
	5,6,	//optimistic
	1,2,	//Sadistic
};

const uint8_t normal_bmi_male[5][11] PROGMEM = {
  { 40,0x00,0x11,0x11,0x22,0x23,0x33,0x44,0x45,0x56,0x68 },
  { 50,0x00,0x01,0x11,0x12,0x22,0x23,0x34,0x44,0x55,0x57 },
  { 60,0x00,0x00,0x11,0x11,0x22,0x23,0x33,0x44,0x45,0x56 },
  { 70,0x00,0x11,0x11,0x12,0x22,0x23,0x33,0x44,0x44,0x00 },
  { 200,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05 }
};

const uint8_t normal_bmi_female[6][11] PROGMEM = {
  { 30,0x01,0x11,0x12,0x23,0x33,0x44,0x55,0x66,0x77,0x8D },
  { 40,0x01,0x11,0x12,0x22,0x33,0x34,0x45,0x56,0x66,0x7B },
  { 50,0x00,0x01,0x11,0x22,0x23,0x33,0x44,0x45,0x56,0x6A },
  { 60,0x00,0x00,0x01,0x11,0x12,0x22,0x33,0x34,0x44,0x57 },
  { 70,0x00,0x00,0x00,0x11,0x11,0x22,0x22,0x23,0x33,0x35 },
  { 200,0x00,0x00,0x00,0x00,0x00,0x11,0x11,0x11,0x11,0x22 }
};

const uint8_t sadistic_bmi_male[1][11] PROGMEM = {
  { 200,0x00,0x00,0x00,0x11,0x11,0x11,0x22,0x22,0x23,0x34 }
};

const uint8_t sadistic_bmi_female[2][11] PROGMEM = {
  { 30,0x00,0x00,0x01,0x11,0x11,0x22,0x22,0x33,0x33,0x46 },
  { 200,0x00,0x00,0x01,0x11,0x11,0x12,0x22,0x23,0x33,0x35 }
};

const uint8_t smoking_table_offset[] = {
	0,10,	//normal
	0,10,	//pessimistic
	0,10,	//optimistic
	19,21,	//Sadistic
};

const uint8_t smoking_table_count[] = {
	10,9,	//normal
	10,9,	//pessimistic
	10,9,	//optimistic
	2,2,	//Sadistic
};

const uint8_t normal_smoking_male[10][2] PROGMEM = {
  { 30,171 },
  { 35,169 },
  { 40,160 },
  { 45,158 },
  { 50,151 },
  { 55,142 },
  { 60,128 },
  { 65,112 },
  { 70,100 },
  { 200,75 }
};

const uint8_t normal_smoking_female[9][2] PROGMEM = {
  { 30, 148 },
  { 45, 146 },
  { 50, 143 },
  { 55, 141 },
  { 60, 137 },
  { 65, 111 },
  { 70, 114 },
  { 75, 86 },
  { 200, 69 }
};

const uint8_t sadistic_smoking_male[2][2] PROGMEM = {
  { 30, 80 },
  { 200, 78 }
};

const uint8_t sadistic_smoking_female[2][2] PROGMEM = {
  { 30, 89 },
  { 200, 87 }
};


const uint8_t day_in_month[12] PROGMEM = {31,28,31,30,31,30,31,31,30,31,30,31};

uint8_t is_leap_year ( uint16_t year )
{
  if ((year % 400) == 0)
    return 1;
  if ((year % 100) == 0)
    return 0;
  if ((year % 4) == 0)
    return 1;
  return 0;
}

uint32_t date_diff ( uint8_t month1, uint8_t day1, uint8_t year1, uint8_t month2, uint8_t day2, uint8_t year2 )
{
  uint32_t diff = 0;
  int i;

  if((year2 < year1) || ((year2 == year1) && (month2 < month1)))
    return date_diff( month2, day2, year2, month1, day1, year1 ) * -1;

  if((month1 == month2) && (year1 == year2))
    return day2 - day1;
  if(year1==year2)
  {
    diff = pgm_read_byte(&day_in_month[month1-1]) - day1;
    if(month1 == 2)
      diff += is_leap_year(year1);
    for(i=month1+1;i<month2;i++)
    {
      diff+=pgm_read_byte(&day_in_month[i-1]);
      if(i==2)
        diff+=is_leap_year(year1);
    }
    diff += day2;
    return diff;
  }
  diff = pgm_read_byte(&day_in_month[month1-1]) - day1;
  if(month1 == 2)
      diff+=is_leap_year(year1);
  for(i=month1+1;i<=12;i++)
  {
    diff+=pgm_read_byte(&day_in_month[i-1]);
    if(i==2)
      diff+=is_leap_year(year1);
  }
  for(i=year1+1;i<year2;i++)
    diff+=365+is_leap_year(i);
  for(i=1;i<month2;i++)
  {
    diff+=pgm_read_byte(&day_in_month[i-1]);
    if(i==2)
      diff+=is_leap_year(year2);
  }
  diff += day2;
  return diff;
}

uint8_t BodyMassIndex ( uint8_t unit, uint16_t height, uint16_t weight )
{
  uint32_t bmi;
  if ( unit == BMI_Imperial )
  {
    //Imperial, Weight in pounds, Height in inches
    //bmi = (weight * 703) / (height * height);
    bmi = weight;
    bmi *= 703;
    bmi /= height;
    bmi /= height;
    return (bmi > 255) ? 255 : bmi;
  }
  else if ( unit == BMI_Metric )
  {
    //Metric, Weight in Kilograms, Height in centimeters
    //bmi = (weight * 10000) / (height * height);
    bmi = weight;
    bmi *= 10000;
    bmi /= height;
    bmi /= height;
    return (bmi > 255) ? 255 : bmi;
  }
  else
  {
    //User knows their BMI, so it is entered directly.
    return weight & 0xFF;
  }
}

uint32_t random_days(uint8_t hour, uint8_t min, uint8_t sec, uint32_t base_days)
{
	init_crand_consistent(hour,min,sec);
	uint32_t random = (crand(0) * 1000) / 0x7FFF;
	return (uint32_t)((base_days * random) / 1000);
}

uint32_t days_table[] = {
//  male,   female
	269450, 289590,	//normal
	214662, 234802, //pessimistic
	305975, 326115, //optimistic
	134307, 143490, //sadistic
};

uint32_t ETD ( uint8_t DOB_month, 
               uint8_t DOB_day, 
               uint8_t DOB_year, 
               uint8_t month, 
               uint8_t day, 
               uint8_t year, 
               uint8_t Gender, 
               uint8_t Mode, 
               uint8_t BMI, 
               uint8_t Smoker, 
               uint8_t hour,
               uint8_t min,
               uint8_t sec)
{
  int y,i,bmi;
  uint32_t diff;
  uint32_t random;
  int32_t days;
  
  diff = date_diff(DOB_month,DOB_day,DOB_year,month,day,year);
  y = (diff * 10) / 3653;
  
  days = days_table[(Mode * 2) + Gender];
  days -= diff*10;
  bmi = BMI;
  if(bmi > 45)
    bmi = 45;
  if(bmi < 26)
    bmi = 26;
  bmi -= 26;
  if (Mode == DC_mode_optimistic) {
    days += random_days(hour,min,sec,54790);
  } else if (Mode == DC_mode_pessimistic) {
    days -= random_days(hour,min,sec,36530);
  }
  
  if(y>=20)
  {
  	  for(i=0;i<bmi_table_count[(Mode * 2) + Gender];i++)
  	  {
  	  	  if ( y < pgm_read_byte(&normal_bmi_male[bmi_table_offset[(Mode * 2) + Gender]+i][0]) )
          {
          	uint8_t temp = pgm_read_byte(&normal_bmi_male[bmi_table_offset[(Mode * 2) + Gender]+i][(bmi/2)+1]);
          	if(bmi&1)
          		temp &= 0x0F;
          	else
          		temp >>= 4;
            days -= (uint16_t)(temp * 3653);
            break;
          }
  	  }
  }
  if((y>=25)&&(Smoker == DC_smoker))
  {
      for(i=0;i<smoking_table_count[(Mode * 2) + Gender];i++)
      {
        if( y < pgm_read_byte(&normal_smoking_male[smoking_table_offset[(Mode * 2) + Gender]+i][0]) )
        {
          days -= (uint16_t)(pgm_read_byte(&normal_smoking_male[smoking_table_offset[(Mode * 2) + Gender]+i][1])*160);
          break;
        }
      }
  }
  if (days < 0) return 0;
  days *= 144;	//Convert Days into Minutes left.
  return days;
}


/*
	uint8_t EEMEM EE_DOB_MONTH = 11; //Death Clock variables are preserved in the event of an extended power outage.
  uint8_t EEMEM EE_DOB_DAY = 14;
  uint8_t EEMEM EE_DOB_YEAR = 80;
  uint8_t EEMEM EE_SET_MONTH = 7;
  uint8_t EEMEM EE_SET_DAY = 28;
  uint8_t EEMEM EE_SET_YEAR = 110;
  uint8_t EEMEM EE_GENDER = DC_gender_male;
  uint8_t EEMEM EE_DC_MODE = DC_mode_normal;
  uint8_t EEMEM EE_BMI_UNIT = BMI_Imperial;
  uint16_t EEMEM EE_BMI_WEIGHT = 400;
  uint16_t EEMEM EE_BMI_HEIGHT = 78;
  uint8_t EEMEM EE_SMOKER = DC_non_smoker;
  uint8_t EEMEM EE_SET_HOUR = 20;
  uint8_t EEMEM EE_SET_MIN = 05;
  uint8_t EEMEM EE_SET_SEC = 25;
*/
uint8_t eeprom_data[17];
uint32_t load_raw_etd(void)
{
  uint8_t i;
  for(i=0;i<17;i++)
  	  eeprom_data[i] = eeprom_read_byte(&EE_DOB_MONTH + i);
  dc_mode = eeprom_data[7];
  return ETD(  eeprom_data[0],eeprom_data[1],eeprom_data[2]+1900,
  	  			eeprom_data[3],eeprom_data[4],eeprom_data[5]+1900,
  	  			eeprom_data[6],eeprom_data[7],
  	  			BodyMassIndex ( eeprom_data[8], (eeprom_data[9] << 8) | eeprom_data[10], (eeprom_data[11] << 8) | eeprom_data[12]),
  	  			eeprom_data[13],eeprom_data[14],eeprom_data[15],eeprom_data[16] );
  	  /*eeprom_read_byte(&EE_DOB_MONTH),
                              eeprom_read_byte(&EE_DOB_DAY),
                              eeprom_read_byte(&EE_DOB_YEAR)+1900,
                              eeprom_read_byte(&EE_SET_MONTH),
                              eeprom_read_byte(&EE_SET_DAY),
                              eeprom_read_byte(&EE_SET_YEAR)+1900,
                              eeprom_read_byte(&EE_GENDER),
                              dc_mode,
                              BodyMassIndex( eeprom_read_byte(&EE_BMI_UNIT), eeprom_read_word(&EE_BMI_HEIGHT), eeprom_read_word(&EE_BMI_WEIGHT)),
                              eeprom_read_byte(&EE_SMOKER),
                              eeprom_read_byte(&EE_SET_HOUR),
                              eeprom_read_byte(&EE_SET_MIN),
                              eeprom_read_byte(&EE_SET_SEC));*/
}

void load_etd(void)
{
  uint32_t result = load_raw_etd();
      result -= date_diff( eeprom_data[3],eeprom_data[4],eeprom_data[5]+1900,
      	  					/*eeprom_read_byte(&EE_SET_MONTH),
                           eeprom_read_byte(&EE_SET_DAY),
                           eeprom_read_byte(&EE_SET_YEAR)+1900, */
                           date_m,date_d,date_y+2000) * 1440l * ((dc_mode == DC_mode_sadistic)?4:1);
  result -= (time_h * 60) * ((dc_mode == DC_mode_sadistic)?4:1);
  result -= (time_m) * ((dc_mode == DC_mode_sadistic)?4:1);
  minutes_left = (int32_t)result;
  calc_death_date();
  if(death_y < (date_y + 100))	//Bug fix for the rare cases where Minutes left is inadvertantly positive, when it should not be.
  	  minutes_left = 0;
  else if((death_y == (date_y + 100)) && (death_m < date_m))
  	  minutes_left = 0;
  else if ((death_y == (date_y + 100)) && (death_m == date_m) && (death_d < date_d))
  	  minutes_left = 0;
  if(minutes_left <= 0)
  	  reaper_tow_rip=1;
  else
  	  reaper_tow_rip=0;
}

void calc_death_date(void)
{
	uint32_t timeleft;
	death_m = eeprom_read_byte(&EE_SET_MONTH);
	death_d = eeprom_read_byte(&EE_SET_DAY);
	death_y = eeprom_read_byte(&EE_SET_YEAR);
	timeleft = load_raw_etd();
	
	while (timeleft >= 1440)
      {
        timeleft -= 1440;
        death_d++;  
        if ((death_d > 31) ||
               ((death_d == 31) && ((death_m == 4)||(death_m == 6)||(death_m == 9)||(death_m == 11))) ||
               ((death_d == 30) && (death_m == 2)) ||
               ((death_d == 29) && (death_m == 2) && !leapyear(1900+death_y))) {
                 death_d = 1;
                 death_m++;
            }
            if(death_m > 12)
            {
              death_m=1;
              death_y++;
            } 
      }
}

#endif