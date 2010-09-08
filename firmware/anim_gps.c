#include <avr/io.h>      // this contains all the IO port definitions
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "util.h"
#include "ratt.h"
#include "ks0108.h"
#include "glcd.h"

#ifdef GPSENABLE
extern volatile uint8_t gpsenable;
extern volatile int8_t timezone;
extern volatile uint8_t just_pressed;
extern volatile uint8_t displaystyle;
extern volatile uint8_t displaymode;

uint8_t GPS_Text[] PROGMEM = "\x0A" "GPS Setup\0"
	                 "\x01" "\0"
	                 "\x0B" "Offset  : \0"
	                 "\x11" "Status  : NOLOCK\0"
	                 "\x0B" "GPS Date: \0"
	                 "\x15" "GPS Time:        UTC\0"
	                 "\x01" "\0"
	                 "\x14" "Menu=Exit, Set=-, +";
	                 

void initanim_GPS(void){
 uint8_t UpdateTZ =1;
 uint8_t i,j;
 timezone=(int8_t)eeprom_read_byte(&EE_TIMEZONE);
 glcdClearScreen();
 for(i=0,j=0;i<8;i++)
 {
 	 if(i) glcdSetAddress(MENU_INDENT, i);
     glcdPutStr_rom(&GPS_Text[j+1] ,NORMAL);
     j+=pgm_read_byte(&GPS_Text[j])+1;
 }
 while (1) {
  // Must get a good test to enable gps
  // read buttons
  if (just_pressed) {
   switch (just_pressed) {
    case 4: if (++timezone>56) {timezone=56;}
            UpdateTZ=1;
            break;
    case 2: if (--timezone<-48) {timezone=-48;}
            UpdateTZ=1;
            break;
    case 1: displaystyle=eeprom_read_byte(&EE_STYLE); glcdClearScreen(); displaymode = CFG_MENU; return;
   }
   just_pressed=0;
  }
  // display
  if (UpdateTZ) {
   UpdateTZ=0;
   eeprom_write_byte(&EE_TIMEZONE, timezone);
   glcdSetAddress(45+MENU_INDENT,2);
   if(timezone<0)
   	   glcdPutStr("-",NORMAL);
   else
   	   glcdPutStr("+",NORMAL);
   //glcdPutStr((timezone<0 ? "-" : "+"),NORMAL);
   printnumber(TIMEZONEHOUR,NORMAL);
   glcdPutStr(":",NORMAL);
   printnumber(TIMEZONEMIN,NORMAL);
   
  }
  // read gps 
  //GPSRead(1); //1 if debugging to screen on line 5
  //Hooked GPSRead elsewhere, that is quick enough for actual use.
  // display
  //_delay_ms(500);
 }
}
#endif
