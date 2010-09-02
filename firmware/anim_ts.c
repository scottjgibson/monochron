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
#include "font5x7.h"
#include "fonttable.h"

#ifdef TSCHRON

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t old_m, old_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarming, alarm_h, alarm_m;
extern volatile uint8_t time_format;
extern volatile uint8_t region;
extern volatile uint8_t score_mode;
extern volatile uint8_t RotateFlag;
extern volatile uint8_t second_changed, minute_changed, hour_changed;

#ifdef OPTION_DOW_DATELONG
extern const uint8_t DOWText[]; 
extern const uint8_t MonthText[]; 
#endif

//Prototypes:
//Called from dispatcher:
void initamin_ts(void);
void initdisplay_ts(uint8_t inverted);
void drawdisplay_ts(uint8_t inverted);
void step_ts(void);

//Support:
void drawdot_ts(uint8_t x, uint8_t y, uint8_t inverted);
void draw7seg_ts(uint8_t x, uint8_t y, uint8_t segs, uint8_t inverted);
uint8_t digitsize_ts(uint8_t d);
uint8_t drawdigit_ts(uint8_t x, uint8_t y, uint8_t d, uint8_t inverted);
void drawsegment_ts(uint8_t s, uint8_t x, uint8_t y, uint8_t inverted);
void drawvseg_ts(uint8_t x, uint8_t y, uint8_t inverted);
void drawhseg_ts(uint8_t x, uint8_t y, uint8_t inverted);
void drawseg_ts(uint8_t x, uint8_t y, uint8_t inverted, uint8_t width, uint8_t height);
void setstring_ts(void);
void setstringdigits_ts(uint8_t, uint8_t);



//                                0123456789012345678
//                                12:15:05x09-01-10x 
volatile uint8_t dispstring[19]={"12:15:05 09-01-10 "};
volatile uint8_t posx=0;     // current x offset (negative)
volatile uint8_t posstr=0;   // current 1st char visible
volatile uint8_t scrx=0;     // our current screen display position, if 0, then first char could be a partial char.

void initanim_ts(void) {
#ifdef DEBUGF
  DEBUG(putstring("screen width: "));
  DEBUG(uart_putw_dec(GLCD_XPIXELS));
  DEBUG(putstring("\n\rscreen height: "));
  DEBUG(uart_putw_dec(GLCD_YPIXELS));
  DEBUG(putstring_nl(""));
#endif
  initdisplay_ts(0);
}

void initdisplay_ts(uint8_t inverted) {
  glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);
  posx=0; 
  posstr=0;
  //setstringdigits_ts(15,20);
  setstring_ts();
  drawdisplay_ts(inverted);
}

void setstring_ts()
{
 // 01234567890123456789
 // 12:15:05x09-01-2010x 
 setstringdigits_ts(0,(score_mode == SCORE_MODE_ALARM ? alarm_h : time_h));
 setstringdigits_ts(3,(score_mode == SCORE_MODE_ALARM ? alarm_m : time_m));
 setstringdigits_ts(6,(score_mode == SCORE_MODE_ALARM ? 0 : time_s));
 setstringdigits_ts((region == REGION_US ? 9 : 12),date_m);
 setstringdigits_ts((region == REGION_US ? 12 : 9),date_d);
 setstringdigits_ts(15,date_y);
}

void setstringdigits_ts(uint8_t cpos, uint8_t val)
{
 dispstring[cpos]=(val/10);
 dispstring[cpos+1]=(val%10);
}

void drawdisplay_ts(uint8_t inverted) {
 static uint8_t loop=0;
 if (++loop<6) return;
 loop=0; 
 glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted); 
 uint8_t tx=0;
 uint8_t cx=0;
 uint8_t dx=0;
 uint8_t sx=posstr;
 setstring_ts();
 while (tx < 128) {
  dx = digitsize_ts(dispstring[sx]);
  cx = drawdigit_ts(tx, 0, dispstring[sx++], inverted);
  if (sx>17) sx=0;
  if (!tx) 
   { 
   if (cx==0) 
    {
    posx=0;
    if (++posstr>17) posstr=0;
    }
   else
    posx+=2;
   }
  tx+=cx;
 }
 return; 
}

void step_ts(void) {
	if(!RotateFlag || (minute_changed == 2) || (hour_changed == 2)) {
		minute_changed = hour_changed = 0;
        }
       //posx++;
}

void drawdot_ts(uint8_t x, uint8_t y, uint8_t inverted) {
 drawseg_ts(x,y-(DOTRADIUS/2),inverted, DOTRADIUS, DOTRADIUS);
 //glcdFillCircle(x, y, DOTRADIUS, !inverted);
}

void draw7seg_ts(uint8_t x, uint8_t y, uint8_t segs, uint8_t inverted)
{
	for(uint8_t i=0;i<7;i++)
	{
		if(segs & (1 << (7 - i)))
			drawsegment_ts('a'+i, x, y, inverted);
		//else
			//drawsegment_ts('a'+i, x, y, !inverted);
	}
}

uint8_t digitsize_ts(uint8_t d)
{
	uint8_t rval = HSEGMENT_W+2;
	if(d==':')
		rval = DOTRADIUS + 2;
	else if (d=='-')
		rval = DOTRADIUS + 2;
	return rval;
}

uint8_t drawdigit_ts(uint8_t x, uint8_t y, uint8_t d, uint8_t inverted) {
  scrx=x; // current start of char
  uint8_t rval = HSEGMENT_W+2;
  if(d < 10) {
  	  draw7seg_ts(x,y,eeprom_read_byte(&numbertable[d]),inverted);
  }
  else if (d==':') {
  	  drawdot_ts(x,16,inverted);
          drawdot_ts(x,47,inverted);
          rval= DOTRADIUS + 2;
  }
  else if (d=='-') {
          drawdot_ts(x,31,inverted);
          rval= DOTRADIUS + 2;
  }
  if (!scrx) rval-=posx;
  return rval;
}

void drawsegment_ts(uint8_t s, uint8_t x, uint8_t y, uint8_t inverted) {
  switch (s) {
  case 'a':
    drawhseg_ts(x, y, inverted);
    break;
  case 'b':
    drawvseg_ts( x + HSEGMENT_W - VSEGMENT_W, y, inverted);
    break;
  case 'c':
    drawvseg_ts( x + HSEGMENT_W - VSEGMENT_W, GLCD_YPIXELS/2, inverted);
    break;
  case 'd':
    drawhseg_ts(x, GLCD_YPIXELS-HSEGMENT_H, inverted);
    break;
  case 'e':
    drawvseg_ts(x, GLCD_YPIXELS/2, inverted);
    break;
  case 'f':
    drawvseg_ts(x, y, inverted);
    break;
  case 'g':
    drawhseg_ts(x,(GLCD_YPIXELS/2)-(HSEGMENT_H/2), inverted);
    break;    
  }
}


void drawvseg_ts(uint8_t x, uint8_t y, uint8_t inverted) {
 drawseg_ts(x,y,inverted, VSEGMENT_W, GLCD_YPIXELS/2);
}

void drawhseg_ts(uint8_t x, uint8_t y, uint8_t inverted) {   
 drawseg_ts(x,y,inverted, HSEGMENT_W, HSEGMENT_H);
}

void drawseg_ts(uint8_t px, uint8_t y, uint8_t inverted, uint8_t width, uint8_t height)
{
  int16_t x = px;
  int16_t hSeg = width;
  if (!scrx)
  {
    //if (x + hSeg < posx) return;
    x-=posx;
    if (x<0) 
     {
     hSeg+=x;
     if (hSeg<0) {return;}
     x=0;
     } 
  }

  else if (x + hSeg>127) {hSeg=128-x;}
  if(x > 127) return;
  glcdFillRectangle((uint8_t)x, y, (uint8_t)hSeg, height, !inverted);
}

//#ifdef TSCHRON
#endif
