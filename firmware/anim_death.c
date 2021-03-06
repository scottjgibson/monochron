/* ***************************************************************************
// anim.c - the main animation and drawing code for MONOCHRON
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
**************************************************************************** */

#include <avr/io.h>      // this contains all the IO port definitions
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "util.h"
#include "ratt.h"
#include "ks0108.h"
#include "glcd.h"
#include "deathclock.h"

#ifdef DEATHCHRON

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t old_m, old_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t death_m, death_d, death_y;
extern volatile uint8_t alarming, alarm_h, alarm_m;
extern volatile uint8_t time_format;
extern volatile uint8_t region;
extern volatile uint8_t score_mode;

volatile uint8_t border_tick=0;

extern volatile int32_t minutes_left, old_minutes_left;
extern volatile uint8_t dc_mode;

extern volatile uint8_t reaper_tow_rip;

extern volatile uint8_t last_buttonstate, just_pressed, pressed;

uint16_t left_score, right_score;
int32_t results;

extern volatile uint8_t minute_changed, hour_changed;

volatile uint8_t redraw_time = 0;
volatile uint8_t last_score_mode = 0;

extern const uint8_t skull_on_white[];
extern const uint8_t reaper_on_white[];
extern const uint8_t rip_on_white[];


// special pointer for reading from ROM memory
PGM_P skull0_p PROGMEM = (prog_char *) skull_on_white;
PGM_P reaper0_p PROGMEM = (prog_char *) reaper_on_white;
PGM_P rip0_p PROGMEM = (prog_char *) rip_on_white;

void death_blitsegs_rom(int16_t x_origin, uint8_t y_origin, PGM_P bitmap_p, uint8_t width, uint8_t height, uint8_t inverted);
void drawdisplay_death(uint8_t inverted);


void render_image (uint8_t image, int16_t x, uint8_t inverted)
{
  switch(image)
  {
    default:
    case SKULL:
      if((x > -76) && (x < 128))
        death_blitsegs_rom(x+0,0,skull0_p, 76, 64, inverted);
      break;
    case REAPER:
      if((x > -42) && (x < 128))
        death_blitsegs_rom(x+0,0,reaper0_p, 42, 64, inverted);
      break;
    case RIP:
      if((x > -56) && (x < 128))
        death_blitsegs_rom(x+0,0,rip0_p, 56, 64, inverted);
      break;
    case REAPER_TOW_RIP:
      if((x > -56) && (x <= 36))
        death_blitsegs_rom(x+0,0,rip0_p, 56, 64, inverted);
      if((x > -98) && (x < 212))
        death_blitsegs_rom(x+56,0,reaper0_p, 42, 64, inverted);
      if((x >= -30 ) && (x < -24)) {
        glcdSetAddress(30+x,5);
        glcdWriteChar((death_y%10)+'0', inverted);
      } else if ((x >= -24 ) && (x < -18)) {
        glcdSetAddress(24+x,5);
        glcdWriteChar(((death_y%100)/10)+'0', inverted);
        glcdWriteChar((death_y%10)+'0', inverted);
      } else if ((x >= -18 ) && (x < -12)) {
        glcdSetAddress(18+x,5);
        glcdWriteChar(((19+(death_y/100))%10)+'0', inverted);
        glcdWriteChar(((death_y%100)/10)+'0', inverted);
        glcdWriteChar((death_y%10)+'0', inverted);
      } else if ((x >= -12 ) && (x < 36)) {
        glcdSetAddress(12+x,5);
        glcdWriteChar(((19+(death_y/100))/10)+'0', inverted);
        glcdWriteChar(((19+(death_y/100))%10)+'0', inverted);
        glcdWriteChar(((death_y%100)/10)+'0', inverted);
        glcdWriteChar((death_y%10)+'0', inverted);
      } else if (x >= 36) {
        glcdSetAddress(48,5);
        glcdWriteChar(((19+(death_y/100))/10)+'0', inverted);
        glcdWriteChar(((19+(death_y/100))%10)+'0', inverted);
        glcdWriteChar(((death_y%100)/10)+'0', inverted);
        glcdWriteChar((death_y%10)+'0', inverted);
      }
  }
}


void death_setscore(void)
{
  if(score_mode != last_score_mode) {
    //Death Clock and Death Alarm requires 8 digits to be drawn, while the remaining modes, only require 4 digits.
    //if(((score_mode == SCORE_MODE_DEATH_TIME) || (score_mode == SCORE_MODE_DEATH_ALARM)) && ((last_score_mode != SCORE_MODE_DEATH_TIME) && (last_score_mode != SCORE_MODE_DEATH_ALARM)))
    //  redraw_time = 2;
    //else if(((last_score_mode == SCORE_MODE_DEATH_TIME) || (last_score_mode == SCORE_MODE_DEATH_ALARM)) && ((score_mode != SCORE_MODE_DEATH_TIME) && (score_mode != SCORE_MODE_DEATH_ALARM)))
    //  redraw_time = 2;
    //else
      redraw_time = 1;
    last_score_mode = score_mode;
  }
  switch(score_mode) {
    case SCORE_MODE_TIME:
      if((minute_changed || hour_changed)) {
        if(hour_changed) {
          left_score = old_h;
          right_score = old_m;
        } else if (minute_changed) {
          right_score = old_m;
        }
      } else {
        left_score = time_h;
        right_score = time_m;
      }
      break;
    case SCORE_MODE_DATE:
      if(region == REGION_US) {
        left_score = date_m;
        right_score = date_d;
      } else {
        left_score = date_d;
        right_score = date_m;
      }
      break;
    case SCORE_MODE_YEAR:
      left_score = 20;
      right_score = date_y;
      break;
    case SCORE_MODE_DEATH_TIME:
      if((minute_changed || hour_changed)) {
          left_score = old_minutes_left/10000;
          right_score = old_minutes_left%10000;
      } else {
          //if((minutes_left - ((dc_mode == DC_mode_sadistic)?(time_s/15):0)) > 0)
        left_score = (minutes_left - ((dc_mode == DC_mode_sadistic)?(time_s/15):0))/10000;
        right_score = (minutes_left - ((dc_mode == DC_mode_sadistic)?(time_s/15):0))%10000;
      }
      if(minutes_left <= 0)
          left_score = right_score = 0;
      break;
    case SCORE_MODE_DEATH_DATE:
      if(region == REGION_US) {
        left_score = death_m;
        right_score = death_d;
      } else {
        left_score = death_d;
        right_score = death_m;
      }
      break;
    case SCORE_MODE_DEATH_YEAR:
      left_score = 19 + (death_y / 100);
      right_score = death_y % 100;
      break;
    case SCORE_MODE_ALARM:
      left_score = alarm_h;
      right_score = alarm_m;
      break;
    case SCORE_MODE_DEATH_ALARM:
      results = minutes_left;
      if((time_h > alarm_h) || ((time_h == alarm_h) && (time_m > alarm_m)) || ((time_h == alarm_h) && (time_m == alarm_m) && (time_s > 0)))
        results -= (((((alarm_h * 60) + alarm_m) + 1440) - ((time_h * 60) + time_m)) * ((dc_mode == DC_mode_sadistic)?4:1));
      else
        results -= ((((alarm_h * 60) + alarm_m) - ((time_h * 60) + time_m)) * ((dc_mode == DC_mode_sadistic)?4:1));
      left_score = results / 10000;
      right_score = results % 10000;
      if(results < 0) {
        left_score = 0;
        right_score = 0;
      }
      break;
  }
}

void initanim_death(void) {
  static int16_t scroller = -84;
  glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, scroller==-84);
  for(;scroller<138;scroller++)
  {
    render_image (SKULL,scroller,1);
    delay_ms(16);
    if(scroller==26)
    	delay_ms(2000);
  }
  score_mode = SCORE_MODE_DEATH_TIME;
  minute_changed = hour_changed = 0;
  load_etd();
  initdisplay(0);
}

uint8_t display_digits[8];

void prep_digits(void)
{
	uint8_t i;
	uint16_t temp1=left_score, temp2=right_score;
	if((score_mode != SCORE_MODE_DEATH_TIME) && (score_mode != SCORE_MODE_DEATH_ALARM))
    {
		if ((time_format == TIME_12H) && ((score_mode == SCORE_MODE_TIME) || (score_mode == SCORE_MODE_ALARM)))
	      display_digits[0] = ((left_score + 23)%12 + 1)/10;
	    else 
	      display_digits[0] = left_score/10;
	    
	    if ((time_format == TIME_12H) && ((score_mode == SCORE_MODE_TIME) || (score_mode == SCORE_MODE_ALARM)))
	      display_digits[1] = ((left_score + 23)%12 + 1)%10;
	    else
	      display_digits[1] = left_score%10;
	    
	    display_digits[3] = right_score/10;
	    display_digits[4] = right_score%10;
	    
	    if(score_mode == SCORE_MODE_TIME)
	    {
	      if(time_format == TIME_12H)
	      {
	      	  display_digits[2] = ((time_s & 1)?((time_s < 30)?17:16):10) | 0x80;
	      	  display_digits[5] = 12;
	      	  if(time_h < 12)
		      	display_digits[6] = 13;
		      else
		      	display_digits[6] = 14;
		      display_digits[7] = 15;
	      }
	      else
	      {
		      display_digits[2] = display_digits[5] = 10;
		      display_digits[6] = (time_s/10) | 0x80;
		      display_digits[7] = (time_s%10) | 0x80;
		  }
	    }
	    else if (score_mode == SCORE_MODE_ALARM)
	    {
	      display_digits[2] = 10;
	      display_digits[5] = 12;
	      if(time_format == TIME_12H)
	      {
	      	  if(alarm_h < 12)
		      	display_digits[6] = 13;
		      else
		      	display_digits[6] = 14;
		      display_digits[7] = 15;
		  }
		  else
		  {
		  	  display_digits[6] = display_digits[7] = 12;
		  }
	    }
	    else if ((score_mode == SCORE_MODE_DATE) || (score_mode == SCORE_MODE_DEATH_DATE))
	    {
	    	display_digits[6] = display_digits[4];
	    	display_digits[5] = display_digits[3];
	    	display_digits[4] = 11;
	    	display_digits[3] = display_digits[1];
	    	display_digits[2] = display_digits[0];
	    	display_digits[0] = display_digits[1] = display_digits[7] = 12;
	    }
	    else if ((score_mode == SCORE_MODE_YEAR) || (score_mode == SCORE_MODE_DEATH_YEAR))
	    {
	    	display_digits[6] = display_digits[4];
	    	display_digits[5] = display_digits[3];
	    	display_digits[4] = display_digits[1];
	    	display_digits[3] = display_digits[0];
	    	display_digits[0] = display_digits[1] = display_digits[2] = display_digits[7] = 12;
	    }
	}
	else
    {
	    if((left_score != 0) || (right_score != 0))
	    {
	    	for(i=0;i<4;i++,temp1/=10,temp2/=10)
	    	{
	    		//drawbigdigit(DISPLAY_DL4_X_DEATH - i, DISPLAY_TIME_Y_DEATH, temp1 % 10, inverted);
	    		//drawbigdigit(DISPLAY_DR4_X_DEATH - i, DISPLAY_TIME_Y_DEATH, temp2 % 10, inverted);
	    		display_digits[3-i] = temp1 % 10;
	    		display_digits[7-i] = temp2 % 10;
	    	}
	    }
    }
}

void initdisplay_death(uint8_t inverted) {
  int16_t i;
  glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);
  death_setscore();
  prep_digits();
  // time
  if(score_mode == SCORE_MODE_TIME)
  {
  	  display_digits[2] = 10;
  	  if(time_format == TIME_24H)
  	  {
  	  	  display_digits[6] = time_s / 10;
  	  	  display_digits[7] = time_s % 10;
  	  }
  }
  if(((score_mode != SCORE_MODE_DEATH_TIME) && (score_mode != SCORE_MODE_DEATH_ALARM)) ||
  ((left_score != 0) || (right_score != 0))) {
    for(i=0;i<8;i++)
    	drawbigdigit(DISPLAY_H10_X_DEATH + (i*15), DISPLAY_TIME_Y_DEATH, display_digits[i], inverted);
  }
  else
  {
    
      calc_death_date();
      if(!reaper_tow_rip)
      {
        reaper_tow_rip = 1;
        for(i=-108;i<82;i++)
        {
          render_image(REAPER_TOW_RIP,i,inverted);
          delay_ms(16);
        }
      }
      
      render_image(RIP,36,inverted);
      glcdSetAddress(48, 5);
      glcdWriteChar(((19+(death_y/100))/10)+'0', NORMAL);
      glcdWriteChar(((19+(death_y/100))%10)+'0', NORMAL);
      glcdWriteChar(((death_y%100)/10)+'0', NORMAL);
      glcdWriteChar((death_y%10)+'0', NORMAL);
  }

  //drawmidline(inverted);
}

int16_t reaper_x;
#define reaper_y 0
#define reaper_w 56
#define reaper_h 64
void step_death(void) {
  uint8_t i;
  death_setscore();
  if((score_mode == SCORE_MODE_TIME) || (score_mode == SCORE_MODE_DEATH_TIME))
  {
    if(minute_changed) 
    {
      redraw_time = 1;
      minutes_left -= ((dc_mode == DC_mode_sadistic)?4:1);
      minute_changed = 0;
      death_setscore();
    }
    if(hour_changed) 
    {
      minutes_left -= ((dc_mode == DC_mode_sadistic)?4:1);
      initdisplay_death(1);
      for(reaper_x = -52;reaper_x<138;reaper_x++)
      {
        //redraw_time = 1;
        //if((reaper_x%8)==0)
        //  draw(1);
        render_image (REAPER,reaper_x+1,1);
        delay_ms(16);
        if(((reaper_x%15)==6)&&(reaper_x>6))
        {
        	prep_digits();
        	for(i=0;(i<4)&&(((reaper_x/15)+i)<8);i++)
        	{
        		display_digits[(reaper_x/15)+i] |= 0x40;
        	}
            redraw_time = 1;
            drawdisplay_death(1);
        }
        if(reaper_x==43) 
        {
          //render_image (REAPER,reaper_x+1,1);
          delay_ms(500);
          hour_changed = 0;
          death_setscore();
          hour_changed = 1;
          prep_digits();
          for(i=0;(i<4)&&(((reaper_x/15)+i)<8);i++)
          	display_digits[(reaper_x/15)+i] |= 0x40;
          redraw_time = 1;
          drawdisplay_death(1);
          delay_ms(500);
        }
      }
      hour_changed = 0;
      initdisplay_death(0);
    }
  }
}

static uint8_t border_x=0, border_y=0, border_state=0,border_on=0;
void next_border(void)
{
  if(!border_on)
  	  return;
  glcdFillRectangle(border_x, border_y, 2, 2, (border_state<2));
  if(++border_state >= 4) border_state = 0;
  if((border_x == 0) && (border_y == 0))
  {
    border_state += 2;
    if(border_state >= 4) border_state = 0;
    border_y+=2;
  }
  else if ((border_x == 0) && (border_y < 62))
    border_y+=2;
  else if ((border_x == 0) && (border_y == 62))
    border_x+=2;
  else if ((border_x < 126) && (border_y == 62))
    border_x+=2;
  else if ((border_x == 126) && (border_y == 62))
    border_y-=2;
  else if ((border_x == 126) && (border_y > 0))
    border_y-=2;
  else if ((border_x == 126) && (border_y == 0))
    border_x-=2;
  else if ((border_x > 0) && (border_y == 0))
    border_x-=2;
}

void drawdisplay_death(uint8_t inverted) {
   // draw time
   volatile uint8_t redraw_digits = 0;
   static volatile uint8_t old_seconds, old_border_tick;
   uint8_t i;
   TIMSK2 = 0;  //Disable Timer 2 interrupt, to prevent a race condition.
   if(redraw_time)
   {
     //if(redraw_time == 2)
     //  initdisplay(inverted);
     //else
       redraw_digits = 1;
     redraw_time = 0;
   }
   TIMSK2 = _BV(TOIE2); //Race issue gone, renable.
    
    // redraw 10's of hours
  if(!hour_changed)
  	prep_digits();
  if(((score_mode != SCORE_MODE_DEATH_TIME) && (score_mode != SCORE_MODE_DEATH_ALARM)) ||
  	  (minutes_left > 0)) {
    if(reaper_x == 256) {
      reaper_x--;
      initdisplay(inverted);
    }
    
    for(i=0;i<8;i++)
    {
    	if (redraw_digits || intersectrect(reaper_x, reaper_y, reaper_w, reaper_h,
    	DISPLAY_H10_X_DEATH + (i*15), DISPLAY_TIME_Y_DEATH, DISPLAY_DIGITW, DISPLAY_DIGITH) ||
    	(display_digits[i] & 0x80)) {
    		drawbigdigit(DISPLAY_H10_X_DEATH + (i*15), DISPLAY_TIME_Y_DEATH, display_digits[i] & 0x7F, inverted);
    	}
    }
    
    if(score_mode >= SCORE_MODE_DEATH_TIME)
    {
      border_on = 1;
      if(border_tick != old_border_tick)
      {
        next_border();
      }
    }
    else
    {
    	uint8_t current_x = border_x, current_y = border_y;
    	do {
	    	border_state = 3;
	    	next_border();
    	} while((border_x != current_x) || (border_y != current_y));
    	border_on = 0;
    }
  }
  else
  {
    if(hour_changed)
      hour_changed = 0;
    if (redraw_digits || (reaper_x < 256)) {
      reaper_x = 256; //Stop drawing the reaper, already dead. :)
      initdisplay_death(inverted);
    }
  }
  old_border_tick = border_tick;
  old_seconds = time_s;
  redraw_digits = 0;
}

#define DIGIT_WIDTH_DEATH 76
#define DIGIT_HEIGHT_DEATH 64

void death_bitblit_ram(int16_t x_origin, uint8_t y_origin, uint8_t width, uint8_t *bitmap_p, uint16_t size, uint8_t inverted) {
  uint8_t xx,y, p;
  int16_t x;

  if((x_origin+width+1)<0)
    return;
  for (uint16_t i = 0; i<size; i++) {
    p = bitmap_p[i];
    
    x = i % width;
    if (x == 0) {
      while((x+x_origin)<0)
      {
        i++;
        p = bitmap_p[i];
        x = i % width;
      }
      xx = x+x_origin;
      y = i / width;
      //if(((x+x_origin)>=0) && ((x+x_origin)<128))
      glcdSetAddress(xx, (y_origin/8)+y);
      //else
      //   continue;
    }
    if((x+x_origin)<128)
    {
      if (inverted) 
        glcdDataWrite(~p);  
      else 
        glcdDataWrite(p);  
    }
  }
}

// number of segments to expect
#define SEGMENTS 2

void death_blitsegs_rom(int16_t x_origin, uint8_t y_origin, PGM_P bitmap_p, uint8_t width, uint8_t height, uint8_t inverted) {
  uint8_t bitmap[DIGIT_WIDTH_DEATH * DIGIT_HEIGHT_DEATH / 8] = {0};
  
  if(width > DIGIT_WIDTH_DEATH)
  	  return;
  if((x_origin + width) < 0)
    return;
  if(x_origin >= 128)
    return;
  if((y_origin + DIGIT_HEIGHT_DEATH) < 0)
    return;
  if(y_origin >= 64)
    return;
  
  uint16_t pointer=0;
  for (uint8_t line = 0; line<height; line++) {
    uint8_t count = pgm_read_byte(bitmap_p+pointer);
    pointer++;
    while(count--) {
      uint8_t start = pgm_read_byte(bitmap_p+pointer);pointer++;
      uint8_t stop = pgm_read_byte(bitmap_p+pointer);pointer++;
    
      while (start <= stop) {
        bitmap[start + (line/8)*width ] |= _BV(line%8);
        start++;
      }
    }
  }
  death_bitblit_ram(x_origin, y_origin, width, bitmap, DIGIT_HEIGHT_DEATH*width/8, inverted);
}
#endif
