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

const uint8_t normal_bmi_male[6][23] PROGMEM = {
  { 20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 40,0,0,0,0,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,6,6,8 },
  { 50,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,4,4,4,5,5,5,7 },
  { 60,0,0,0,0,0,0,1,1,1,1,2,2,2,3,3,3,4,4,4,5,5,6 },
  { 70,0,0,0,0,1,1,1,1,1,2,2,2,2,3,3,3,4,4,4,4,0,0 },
  { 200,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5 }
};

const uint8_t normal_bmi_female[7][23] PROGMEM = {
  { 20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 30,0,0,0,1,1,1,1,2,2,3,3,3,4,4,5,5,6,6,7,7,8,13 },
  { 40,0,0,0,1,1,1,1,2,2,2,3,3,3,4,4,5,5,6,6,6,7,11 },
  { 50,0,0,0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,6,6,10 },
  { 60,0,0,0,0,0,0,0,1,1,1,1,2,2,2,3,3,3,4,4,4,5,7 },
  { 70,0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,2,3,3,3,3,5 },
  { 200,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2 }
};

const uint8_t sadistic_bmi_male[2][23] PROGMEM = {
  { 20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 200,0,0,0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,2,2,3,3,4 }
};

const uint8_t sadistic_bmi_female[3][23] PROGMEM = {
  { 20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 30,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,2,3,3,3,3,4,6 },
  { 200,0,0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,2,3,3,3,3,5 }
};

const uint16_t normal_smoking_male[11][2] PROGMEM = {
  { 25,0 },
  { 30,2739 },
  { 35,2703 },
  { 40,2557 },
  { 45,2520 },
  { 50,2410 },
  { 55,2265 },
  { 60,2046 },
  { 65,1790 },
  { 70,1607 },
  { 200,1205 }
};

const uint16_t normal_smoking_female[10][2] PROGMEM = {
  { 25, 0 },
  { 30, 2367 },
  { 45, 2331 },
  { 50, 2294 },
  { 55, 2257 },
  { 60, 2192 },
  { 65, 1782 },
  { 70, 1826 },
  { 75, 1381 },
  { 200, 1096 }
};

const uint16_t sadistic_smoking_male[3][2] PROGMEM = {
  { 25, 0 },
  { 30, 1278 },
  { 200, 1242 }
};

const uint16_t sadistic_smoking_female[3][2] PROGMEM = {
  { 25, 0 },
  { 30, 1424 },
  { 200, 1388 }
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
  
  if((Mode == DC_mode_optimistic) || (Mode == DC_mode_pessimistic))
  {
    init_crand();
  }
  
  diff = date_diff(DOB_month,DOB_day,DOB_year,month,day,year);
  y = (diff * 10) / 3653;
  if (Gender == DC_gender_male)
    days = 269450;  //Divide by 100 at the end.
  else
    days = 289590;
  days -= diff*10;
  bmi = BMI;
  if(bmi > 45)
    bmi = 45;
  if(bmi < 24)
    bmi = 24;
  bmi -= 24;
  if(Mode == DC_mode_sadistic)
  {
    if (Gender == DC_gender_male)
    {
      days -= 135143;  //Divide by 100 at the end.
      if ( Smoker == DC_smoker )
        for(i=0;i<3;i++)
        {
          if( y < pgm_read_word(&sadistic_smoking_male[i][0]) )
          {
            days -= (uint16_t)(pgm_read_word(&sadistic_smoking_male[i][1])*10);
            break;
          }
        }
      for(i=0;i<2;i++)
      {
        if ( y < pgm_read_byte(&sadistic_bmi_male[i][0]) )
        {
          days -= (uint16_t)(pgm_read_byte(&sadistic_bmi_male[i][bmi+1]) * 3653);
          break;
        }
      }
    }
    else
    {
      days -= 146100;

        for(i=0;i<3;i++)
        {
          if( y < pgm_read_word(&sadistic_smoking_female[i][0]) )
          {
            days -= (uint16_t)(pgm_read_word(&sadistic_smoking_female[i][1])*10);
            break;
          }
        }
      for(i=0;i<3;i++)
      {
        if ( y < pgm_read_byte(&sadistic_bmi_female[i][0]) )
        {
          days -= (uint16_t)(pgm_read_byte(&sadistic_bmi_female[i][bmi+1]) * 3653);
          break;
        }
      }
}
  }
  else
  {
    if (Gender == DC_gender_male)
    {
      if ( Smoker == DC_smoker )
        for(i=0;i<11;i++)
        {
          if( y < pgm_read_word(&normal_smoking_male[i][0]) )
          {
            days -= (uint16_t)(pgm_read_word(&normal_smoking_male[i][1])*10);
            break;
          }
        }
      for(i=0;i<6;i++)
      {
        if ( y < pgm_read_byte(&normal_bmi_male[i][0]) )
        {
          days -= (uint16_t)(pgm_read_byte(&normal_bmi_male[i][bmi+1]) * 3653);
          break;
        }
      }
    }
    else
    {
      if ( Smoker == DC_smoker )
        for(i=0;i<10;i++)
        {
          if( y < pgm_read_word(&normal_smoking_female[i][0]) )
          {
            days -= (uint16_t)(pgm_read_word(&normal_smoking_female[i][1])*10);
            break;
          }
        }
      for(i=0;i<7;i++)
      {
        if ( y < pgm_read_byte(&normal_bmi_female[i][0]) )
        {
          days -= (uint16_t)(pgm_read_byte(&normal_bmi_female[i][bmi+1]) * 3653);
          break;
        }
      }
    }
  }
  
  if(Mode == DC_mode_optimistic)
  {
    days += 36525;
    random = (crand(0) * 1000) / 0x7FFF;
    days += (uint32_t)((54790 * random) / 1000);
    //days += random(0,5479)*100;
  }
  if(Mode == DC_mode_pessimistic)
  {
    days -= 54788;
    random = (crand(0) * 1000) / 0x7FFF;
    days -= (uint32_t)((36530 * random) / 1000);
    //days -= random(0,3653)*100;
  }
  if (days < 0) return 0;
  //days *= 864;  //Convert days left into seconds left.
  //days /= 10;
  days *= 144;
  return days;
}

uint32_t load_raw_etd(void)
{
  dc_mode = eeprom_read_byte(&EE_DC_MODE);
  return ETD(  eeprom_read_byte(&EE_DOB_MONTH),
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
                              eeprom_read_byte(&EE_SET_SEC));
}

void load_etd(void)
{
  uint32_t result = load_raw_etd();
      result -= date_diff( eeprom_read_byte(&EE_SET_MONTH),
                           eeprom_read_byte(&EE_SET_DAY),
                           eeprom_read_byte(&EE_SET_YEAR)+1900,
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