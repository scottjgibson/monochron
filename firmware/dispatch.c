/* ***************************************************************************
// dispatch.c - Provides switchting between the different chrons
//
// Pretty tricky, just substitutes for the current routine.
// Slides right into the logic without adjustment, sneaky Mr. Data!
//
// This code is distributed under the GNU Public License
// Which can be found at http://www.gnu.org/licenses/gpl.txt
//
**************************************************************************** */

#include <avr/io.h>      // this contains all the IO port definitions
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
 
#include "ratt.h"
#include "dispatch.h"

// 2010-08-07 Version 1 - Dataman

extern volatile displaystyle;



void initanim(void){
 switch (displaystyle) {
 case STYLE_RAT: initanim_rat();
                break;
 case STYLE_INT: initanim_int();
				break;
 case STYLE_SEV: initanim_sev();
				break;
 case STYLE_XDA: initanim_xda();
                break;
 }
}


void initdisplay(uint8_t inverted) {
 switch (displaystyle) {
 case STYLE_RAT: initdisplay_rat(inverted);
                break;
 case STYLE_INT: initdisplay_int(inverted);
				break;
 case STYLE_SEV: initdisplay_sev(inverted); 
				break;
 case STYLE_XDA: initdisplay_xda(inverted);
                 break;
 }
}


void drawdisplay(uint8_t inverted) {
 switch (displaystyle) {
 case STYLE_RAT: drawdisplay_rat(inverted);
                break;
 case STYLE_INT: drawdisplay_int(inverted);
				break;
 case STYLE_SEV: drawdisplay_sev(inverted); 
 				break;
 case STYLE_XDA: drawdisplay_xda(inverted);
                 break;
 }
}


void step(void) {
 switch (displaystyle) {
 case STYLE_RAT: step_rat();
                break;
 case STYLE_INT: step_int();
				break;
 case STYLE_SEV: step_sev(); 
 				break;
 case STYLE_XDA: step_xda();
                 break;
 }
}
