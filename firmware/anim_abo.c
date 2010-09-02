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
extern volatile uint8_t time_s;

#ifdef DEATHCHRON
extern const uint8_t adafruit[];
PGM_P logo_p PROGMEM = (prog_char *) adafruit;
void death_blitsegs_rom(int16_t x_origin, uint8_t y_origin, PGM_P bitmap_p, uint8_t width, uint8_t height, uint8_t inverted);
#endif
                                  
extern uint8_t about[];
//Definition of about[] is in eeprom.c.  Change it there.

void initanim_abo(void){
 uint8_t k, b, line, eof;
 uint16_t ix, lineix;
 ix=0;
#ifdef DEATHCHRON
 glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, 1);
 death_blitsegs_rom(36,0,logo_p, 57, 64, 1);
 uint8_t i = (time_s + 5) % 60;
 while(i != time_s);
#endif
 while (1) {
  glcdClearScreen();
  for (eof=0, lineix=0, line=0; line<8; line++) {
   if (!eof) {
    b = eeprom_read_byte(&about[ix++]);
    if (b==255) {
     eof = 1;
     if (!line) {displaystyle = eeprom_read_byte(&EE_STYLE); initanim(); return;}
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
  delay_ms(500);
 }
}
