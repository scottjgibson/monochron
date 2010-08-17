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

extern volatile uint8_t gpsenable;
extern volatile int8_t timezone;
extern volatile uint8_t just_pressed;


void initanim_GPS(void);

void initanim_GPS(void){
 uint8_t UpdateTZ =1;
 glcdClearScreen();
 glcdPutStr("GPS Setup",NORMAL);
 glcdSetAddress(MENU_INDENT, 2);
 glcdPutStr("Offset:" ,NORMAL);
 glcdSetAddress(MENU_INDENT, 4);
 glcdPutStr("Test: ",NORMAL);
 glcdSetAddress(MENU_INDENT, 7);
 glcdPutStr("Menu=Exit, Set=-, +",NORMAL);
 while (1) {
  // Must get a good test to enable gps
  // read buttons
  if (just_pressed) {
   switch (just_pressed) {
    case 4: if (++timezone>8) {timezone=8;}
            UpdateTZ=1;
            break;
    case 2: if (--timezone<-12) {timezone=-12;}
            UpdateTZ=1;
            break;
    case 1: return;
   }
   just_pressed=0;
  }
  // display
  if (UpdateTZ) {
   UpdateTZ=0;
   glcdSetAddress(45+MENU_INDENT,2);
   glcdPutStr((timezone<0 ? "-" : "+"),NORMAL);
   printnumber(abs(timezone),NORMAL);
  }
  // read gps 
  GPSRead(1); //1 if debugging to screen on line 5
  // display
  //_delay_ms(500);
 }
}
