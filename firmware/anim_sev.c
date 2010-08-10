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


extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t old_m, old_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarming, alarm_h, alarm_m;
extern volatile uint8_t time_format;
extern volatile uint8_t region;
extern volatile uint8_t score_mode;

extern volatile uint8_t second_changed, minute_changed, hour_changed;

#ifdef OPTION_DOW_DATELONG
extern const uint8_t DOWText[] PROGMEM; 
const uint8_t MonthText[] PROGMEM; 
#endif

uint8_t redraw_time = 0;
uint8_t last_score_mode_sev = 0;

//Prototypes:
//Called from dispatcher:
void initamin_sev(void);
void initdisplay_sev(uint8_t inverted);
void drawdisplay_sev(uint8_t inverted);
void step(void);
//Support:
void drawdot_sev(uint8_t x, uint8_t y, uint8_t inverted);
void draw7seg_sev(uint8_t x, uint8_t y, uint8_t segs, uint8_t inverted);
void drawdigit_sev(uint8_t x, uint8_t y, uint8_t d, uint8_t inverted);
void drawsegment_sev(uint8_t s, uint8_t x, uint8_t y, uint8_t inverted);
void drawvseg_sev(uint8_t x, uint8_t y, uint8_t inverted);
void drawhseg_sev(uint8_t x, uint8_t y, uint8_t inverted);



void initanim_sev(void) {
#ifdef DEBUGF
  DEBUG(putstring("screen width: "));
  DEBUG(uart_putw_dec(GLCD_XPIXELS));
  DEBUG(putstring("\n\rscreen height: "));
  DEBUG(uart_putw_dec(GLCD_YPIXELS));
  DEBUG(putstring_nl(""));
#endif
  initdisplay_sev(0);
}

void initdisplay_sev(uint8_t inverted) {
  glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);
}


void drawdisplay_sev(uint8_t pinverted) {
  uint8_t inverted = 0;

  if ((score_mode != SCORE_MODE_TIME) && (score_mode != SCORE_MODE_ALARM))
  {
  	drawdot_sev(GLCD_XPIXELS/2, GLCD_YPIXELS*1/3, !inverted);
    drawdot_sev(GLCD_XPIXELS/2, GLCD_YPIXELS*2/3, !inverted);
    drawdot_sev(GLCD_XPIXELS/2, GLCD_YPIXELS*1/10, !inverted);
  }

  if (score_mode == SCORE_MODE_YEAR) {
    drawdigit_sev(DISPLAY_H10_X, DISPLAY_TIME_Y, 2 , inverted);
    drawdigit_sev(DISPLAY_H1_X, DISPLAY_TIME_Y, 0, inverted);
    drawdigit_sev(DISPLAY_M10_X, DISPLAY_TIME_Y, (date_y % 100)/10, inverted);
    drawdigit_sev(DISPLAY_M1_X, DISPLAY_TIME_Y, date_y % 10, inverted);
  } else if (score_mode == SCORE_MODE_DATE) {
    uint8_t left, right;
    if (region == REGION_US) {
      left = date_m;
      right = date_d;
    } else {
      left = date_d;
      right = date_m;
    }
    drawdigit_sev(DISPLAY_H10_X, DISPLAY_TIME_Y, left/10 , inverted);
    drawdigit_sev(DISPLAY_H1_X, DISPLAY_TIME_Y, left%10, inverted);
    drawdigit_sev(DISPLAY_M10_X, DISPLAY_TIME_Y, right/10, inverted);
    drawdigit_sev(DISPLAY_M1_X, DISPLAY_TIME_Y, right % 10, inverted);
  } 
#ifdef OPTION_DOW_DATELONG
  else if (score_mode == SCORE_MODE_DOW) {
  	uint8_t dow = dotw(date_m, date_d, date_y);
  	draw7seg_sev(DISPLAY_H10_X, DISPLAY_TIME_Y, 0x00 , inverted);
    drawdigit_sev(DISPLAY_H1_X, DISPLAY_TIME_Y, sdotw(dow,0), inverted);
    drawdigit_sev(DISPLAY_M10_X, DISPLAY_TIME_Y, sdotw(dow,1), inverted);
    drawdigit_sev(DISPLAY_M1_X, DISPLAY_TIME_Y, sdotw(dow,2), inverted);
  } else if (score_mode == SCORE_MODE_DATELONG_MON) {
  	draw7seg_sev(DISPLAY_H10_X, DISPLAY_TIME_Y, 0x00 , inverted);
    drawdigit_sev(DISPLAY_H1_X, DISPLAY_TIME_Y, smon(date_m,0), inverted);
    drawdigit_sev(DISPLAY_M10_X, DISPLAY_TIME_Y, smon(date_m, 1), inverted);
    drawdigit_sev(DISPLAY_M1_X, DISPLAY_TIME_Y, smon(date_m,2), inverted);
  } else if (score_mode == SCORE_MODE_DATELONG_DAY) {
  	draw7seg_sev(DISPLAY_H10_X, DISPLAY_TIME_Y, 0x00 , inverted);
    draw7seg_sev(DISPLAY_H1_X, DISPLAY_TIME_Y, 0x00 , inverted);
    drawdigit_sev(DISPLAY_M10_X, DISPLAY_TIME_Y, date_d/10, inverted);
    drawdigit_sev(DISPLAY_M1_X, DISPLAY_TIME_Y, date_d % 10, inverted);
  } 
#endif
  else if ((score_mode == SCORE_MODE_TIME) || (score_mode == SCORE_MODE_ALARM)) {
    // draw time or alarm
    uint8_t left, right;
    if (score_mode == SCORE_MODE_ALARM) {
      left = alarm_h;
      right = alarm_m;
    } else {
      left = time_h;
      right = time_m;
    }
    uint8_t am = (left < 12);
    if (time_format == TIME_12H) {
      left = (left + 23)%12 + 1;
      if(am) {
      	drawdot_sev(GLCD_XPIXELS/2, GLCD_YPIXELS*1/10, inverted);
      } else {
      	drawdot_sev(GLCD_XPIXELS/2, GLCD_YPIXELS*1/10, !inverted);
      }
    }
    else
      drawdot_sev(GLCD_XPIXELS/2, GLCD_YPIXELS*1/10, !inverted);
      

    // draw hours
    if (left >= 10) {
      drawdigit_sev(DISPLAY_H10_X, DISPLAY_TIME_Y, left/10, inverted);
    } else {
      drawdigit_sev(DISPLAY_H10_X, DISPLAY_TIME_Y, 8, !inverted);
    }
    drawdigit_sev(DISPLAY_H1_X, DISPLAY_TIME_Y, left%10, inverted);
    
    drawdigit_sev(DISPLAY_M10_X, DISPLAY_TIME_Y, right/10, inverted);
    drawdigit_sev(DISPLAY_M1_X, DISPLAY_TIME_Y, right%10, inverted);
    
    if (second_changed && time_s%2) {
      drawdot_sev(GLCD_XPIXELS/2, GLCD_YPIXELS*1/3, 0);
      drawdot_sev(GLCD_XPIXELS/2, GLCD_YPIXELS*2/3, 0);
    } else {
      drawdot_sev(GLCD_XPIXELS/2, GLCD_YPIXELS*1/3, 1);
      drawdot_sev(GLCD_XPIXELS/2, GLCD_YPIXELS*2/3, 1);
    }
  }
}


void step_sev(void) {
}

void drawdot_sev(uint8_t x, uint8_t y, uint8_t inverted) {
  glcdFillCircle(x, y, DOTRADIUS, !inverted);
}

void draw7seg_sev(uint8_t x, uint8_t y, uint8_t segs, uint8_t inverted)
{
	for(uint8_t i=0;i<7;i++)
	{
		if(segs & (1 << (7 - i)))
			drawsegment_sev('a'+i, x, y, inverted);
		else
			drawsegment_sev('a'+i, x, y, !inverted);
	}
}

void drawdigit_sev(uint8_t x, uint8_t y, uint8_t d, uint8_t inverted) {
  if(d < 10)
  	  draw7seg_sev(x,y,eeprom_read_byte(&numbertable[d]),inverted);
  else if ((d >= 'a') || (d <= 'z'))
  	  draw7seg_sev(x,y,eeprom_read_byte(&alphatable[(d - 'a')]),inverted);
  else
  	  draw7seg_sev(x,y,0x00,inverted);
}
void drawsegment_sev(uint8_t s, uint8_t x, uint8_t y, uint8_t inverted) {
  switch (s) {
  case 'a':
    drawhseg_sev(x+VSEGMENT_W/2+1, y, inverted);
    break;
  case 'b':
    drawvseg_sev(x+HSEGMENT_W+2, y+HSEGMENT_H/2+2, inverted);
    break;
  case 'c':
    drawvseg_sev(x+HSEGMENT_W+2, y+GLCD_YPIXELS/2+2, inverted);
    break;
  case 'd':
    drawhseg_sev(x+VSEGMENT_W/2+1, GLCD_YPIXELS-HSEGMENT_H, inverted);
    break;
  case 'e':
    drawvseg_sev(x, y+GLCD_YPIXELS/2+2, inverted);
    break;
  case 'f':
    drawvseg_sev(x,y+HSEGMENT_H/2+2, inverted);
    break;
  case 'g':
    drawhseg_sev(x+VSEGMENT_W/2+1, (GLCD_YPIXELS - HSEGMENT_H)/2, inverted);
    break;    
  }
}


void drawvseg_sev(uint8_t x, uint8_t y, uint8_t inverted) {
  glcdFillRectangle(x, y+2, VSEGMENT_W, VSEGMENT_H-4, ! inverted);

  glcdFillRectangle(x+1, y+1, VSEGMENT_W-2, 1, ! inverted);
  glcdFillRectangle(x+2, y, VSEGMENT_W-4, 1, ! inverted);

  glcdFillRectangle(x+1, y+VSEGMENT_H-2, VSEGMENT_W-2, 1, ! inverted);
  glcdFillRectangle(x+2, y+VSEGMENT_H-1, VSEGMENT_W-4, 1, ! inverted);
}


void drawhseg_sev(uint8_t x, uint8_t y, uint8_t inverted) {
  glcdFillRectangle(x+2, y, HSEGMENT_W-4, HSEGMENT_H, ! inverted);

  glcdFillRectangle(x+1, y+1, 1, HSEGMENT_H - 2, ! inverted);
  glcdFillRectangle(x, y+2, 1, HSEGMENT_H - 4, ! inverted);

  glcdFillRectangle(x+HSEGMENT_W-2, y+1, 1, HSEGMENT_H - 2, ! inverted);
  glcdFillRectangle(x+HSEGMENT_W-1, y+2, 1, HSEGMENT_H - 4, ! inverted);
}



