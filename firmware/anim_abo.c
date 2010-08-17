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

extern volatile uint8_t displaystyle;
                                  
extern uint8_t about[];
//Definition of about[] is in eeprom.c.  Change it there.

void initanim_abo(void);

void initanim_abo(){
 uint8_t k, b, line, eof;
 uint16_t ix, lineix;
 ix=0;
 while (1) {
  glcdClearScreen();
  for (eof=0, lineix=0, line=0; line<8; line++) {
   if (!eof) {
    b = eeprom_read_byte(&about[ix++]);
    if (b==255) {
     eof = 1;
     if (!line) {displaystyle = STYLE_RANDOM; initanim(); return;}
     continue;
    } 
    if (!line) {lineix = ix + b;}
    k = ((128 - (b * 6))/2)-1;
    if (k<0) {k=0;}
    glcdSetAddress(k,line);
    for(;b>0;b--) {
     glcdWriteChar(eeprom_read_byte(&about[ix++]),0);
    }
   }
  }
  ix = lineix;
  _delay_ms(500);
 }
}
