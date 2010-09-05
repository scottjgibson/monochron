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



void initanim_GPS(void){
 uint8_t UpdateTZ =1;
 timezone=(int8_t)eeprom_read_byte(&EE_TIMEZONE);
 glcdClearScreen();
 glcdPutStr("GPS Setup",NORMAL);
 glcdSetAddress(MENU_INDENT, 2);
 glcdPutStr("Offset  : " ,NORMAL);
 glcdSetAddress(MENU_INDENT, 3);
 glcdPutStr("Status  : NOLOCK",NORMAL);
 glcdSetAddress(MENU_INDENT, 4);
 glcdPutStr("GPS Date: ",NORMAL);
 glcdSetAddress(MENU_INDENT, 5);
 glcdPutStr("GPS Time:        UTC",NORMAL);
 glcdSetAddress(MENU_INDENT, 7);
 glcdPutStr("Menu=Exit, Set=-, +",NORMAL);
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
