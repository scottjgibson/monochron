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
                                  // 123456789ABCDEF0123456
uint8_t about[] EEMEM =      "\x0a" "MultiChron"
                                  // 123456789ABCDEF0123456
                             "\x0b" "Version 1.0"
                                  // 123456789ABCDEF0123456
                             "\x01" "-"
                                  // 123456789ABCDEF0123456
                             "\x0a" "MultiChron"
                                  // 123456789ABCDEF0123456
	                     "\x0d" "IntruderChron" 
                                  // 123456789ABCDEF0123456
	                     "\x0a" "by Dataman"
                                  // 123456789ABCDEF0123456
	                     "\x0d" "Data Magician"
                                  // 123456789ABCDEF0123456
                             "\x01" "-"
                                  // 123456789ABCDEF0123456
                             "\x0c" "Optimization"
                                  // 123456789ABCDEF0123456
	                     "\x09" "Debugging"
                                  // 123456789ABCDEF0123456
                             "\x0c" "by CaitSith2"
                                  // 123456789ABCDEF0123456
                             "\x0a" "Code Jedi!"
                                  // 123456789ABCDEF0123456
                             "\x01" "-"
                                  // 123456789ABCDEF0123456
	                     "\x09" "RATTChron"
                                  // 123456789ABCDEF0123456
	                     "\x0a" "SevenChron"
                                  // 123456789ABCDEF0123456
                             "\x0b" "XADALICHRON"
                                  // 123456789ABCDEF0123456
                             "\x12" "MonoChron Hardware"
                                  // 123456789ABCDEF0123456
			     "\x0a" "by LadyAda"
                                  // 123456789ABCDEF0123456
			     "\x10" "Simply The Best!"
                                  // 123456789ABCDEF0123456
                             "\x01" "-"
                                  // 123456789ABCDEF0123456
	                     "\x13" "Adafruit Industries" 
                                  // 123456789ABCDEF0123456
                             "\x10" "www.adafruit.com"
                                  // 123456789ABCDEF0123456
                             "\xff";

void initanim_abo(void);

void initanim_abo(){
 uint8_t k, b, line, ix, lineix, eof;
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
