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
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
//#include <math.h>	//For the sin/cos functions.

#include "util.h"
#include "ratt.h"
#include "ks0108.h"
#include "glcd.h"

#ifdef RATTCHRON

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t old_m, old_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarming, alarm_h, alarm_m;
extern volatile uint8_t time_format;
extern volatile uint8_t region;
extern volatile uint8_t score_mode;




uint8_t left_score, right_score;

/*float ball_x, ball_y;
float oldball_x, oldball_y;
float ball_dx, ball_dy;*/
int32_t ball_x, ball_y;
int32_t oldball_x, oldball_y;
int32_t ball_dx, ball_dy;

int32_t rightpaddle_y, leftpaddle_y;
int32_t oldleftpaddle_y, oldrightpaddle_y;
int32_t rightpaddle_dy, leftpaddle_dy;

extern volatile uint8_t minute_changed, hour_changed;

uint8_t redraw_time_rat = 0;
uint8_t last_score_mode_rat = 0;

// Prototypes
// Called by dispatcher
void initamin_rat(void);
void initdisplay_rat(uint8_t);
void drawdisplay_rat(uint8_t);
void step_rat(void);
//Support
void encipher(void);
void init_crand(void);
uint16_t crand(uint8_t);
void setscore_rat(void);
void drawmidline(uint8_t);
uint8_t intersectrect(uint8_t x1, uint8_t y1, uint8_t w1, uint8_t h1, uint8_t x2, uint8_t y2, uint8_t w2, uint8_t h2);
void draw_score_rat(uint8_t redraw_digits, uint8_t inverted);
void drawbigfont(uint8_t x, uint8_t y, uint8_t n, uint8_t inverted);
void drawbigdigit(uint8_t x, uint8_t y, uint8_t n, uint8_t inverted);
//float random_angle_rads(void);
int8_t random_angle(void);
//uint8_t calculate_keepout(float theball_x, float theball_y, float theball_dx, float theball_dy, uint8_t *keepout1, uint8_t *keepout2);
uint8_t calculate_keepout(int32_t theball_x, int32_t theball_y, int32_t theball_dx, int32_t theball_dy, uint32_t *keepout1, uint32_t *keepout2);
uint8_t calculate_dest_pos(uint32_t *left, uint32_t *right, uint32_t *dest, uint8_t dir);


uint8_t dotw(uint8_t mon, uint8_t day, uint8_t yr);

int16_t sine_table[64] = {
	 0x0000,  0x0324,  0x0647,  0x096a,  0x0c8b,  0x0fab,  0x12c8,  0x15e2,
	 0x18f8,  0x1c0b,  0x1f19,  0x2223,  0x2528,  0x2826,  0x2b1f,  0x2e11,
	 0x30fb,  0x33de,  0x36ba,  0x398c,  0x3c56,  0x3f17,  0x41ce,  0x447a,
	 0x471c,  0x49b4,  0x4c3f,  0x4ebf,  0x5133,  0x539b,  0x55f5,  0x5842,
	 0x5a82,  0x5cb4,  0x5ed7,  0x60ec,  0x62f2,  0x64e8,  0x66cf,  0x68a6,
	 0x6a6d,  0x6c24,  0x6dca,  0x6f5f,  0x70e2,  0x7255,  0x73b5,  0x7504,
	 0x7641,  0x776c,  0x7884,  0x798a,  0x7a7d,  0x7b5d,  0x7c29,  0x7ce3,
	 0x7d8a,  0x7e1d,  0x7e9d,  0x7f09,  0x7f62,  0x7fa7,  0x7fd8,  0x7ff6,
};

int16_t sine(int8_t angle)
{
	if(angle == -128) return 0;
	if(angle < 0) return -sine(-angle);
	if(angle == 64) return 32767;
	if(angle < 64) return sine_table[angle];
	return sine_table[63-(angle-65)];
}

int16_t cosine(int8_t angle)
{
	return sine(angle+64);
}

void setscore_rat(void)
{
  if(score_mode != last_score_mode_rat) {
    redraw_time_rat = 1;
    last_score_mode_rat = score_mode;
  }
  switch(score_mode) {
#ifdef OPTION_DOW_DATELONG
  	case SCORE_MODE_DOW:
  	  break;
  	case SCORE_MODE_DATELONG:
  	  right_score = date_d;
  	  break;
#endif
    case SCORE_MODE_TIME:
      if((minute_changed || hour_changed)) {
      	if(hour_changed) {
	      left_score = hours(old_h);
	      right_score = old_m;
	    } else if (minute_changed) {
	      right_score = old_m;
	    }
      } else {
        left_score = hours(time_h);
        right_score = time_m;
      }
      break;
    case SCORE_MODE_DATE:
#ifdef OPTION_DOW_DATELONG
      if((region == REGION_US)||(region == DOW_REGION_US)) {
#else
      if(region == REGION_US) {
#endif
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
    case SCORE_MODE_ALARM:
      left_score = hours(alarm_h);
      right_score = alarm_m;
      break;
  }
}

int16_t ticksremaining;
void initanim_rat(void) {
  DEBUG(putstring("screen width: "));
  DEBUG(uart_putw_dec(GLCD_XPIXELS));
  DEBUG(putstring("\n\rscreen height: "));
  DEBUG(uart_putw_dec(GLCD_YPIXELS));
  DEBUG(putstring_nl(""));

  oldball_x = ball_x = 2500;
  oldball_y = ball_y = 2500;	//Somewhere away from 0,0.
  oldleftpaddle_y = leftpaddle_y = 2500;
  oldrightpaddle_y = rightpaddle_y = 2500;
  init_crand();

  ball_dx = ball_dy = 0;
  initdisplay_rat(0);
}

void initdisplay_rat(uint8_t inverted) {

  glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);
  
  // draw top 'line'
  glcdFillRectangle(0, 0, GLCD_XPIXELS, 2, ! inverted);
  
  // bottom line
  glcdFillRectangle(0, GLCD_YPIXELS - 2, GLCD_XPIXELS, 2, ! inverted);

  // left paddle
  glcdFillRectangle(LEFTPADDLE_X, leftpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H, ! inverted);
  // right paddle
  glcdFillRectangle(RIGHTPADDLE_X, rightpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H, ! inverted);
      
	//left_score = time_h;
	//right_score = time_m;
	setscore_rat();

  // time
    drawbigdigit(DISPLAY_H10_X_RAT, DISPLAY_TIME_Y_RAT, left_score/10, inverted);
    drawbigdigit(DISPLAY_H1_X_RAT, DISPLAY_TIME_Y_RAT, left_score%10, inverted);
  
  drawbigdigit(DISPLAY_M10_X_RAT, DISPLAY_TIME_Y_RAT, right_score/10, inverted);
  drawbigdigit(DISPLAY_M1_X_RAT, DISPLAY_TIME_Y_RAT, right_score%10, inverted);

  drawmidline(inverted);
}

void move_paddle(int32_t *paddle, uint32_t dest) {
	if(abs(*paddle - dest) < MAX_PADDLE_SPEED) {
        *paddle = dest;
      } else {
        if(*paddle > dest)
          *paddle -= MAX_PADDLE_SPEED;
        else
          *paddle += MAX_PADDLE_SPEED;
      }
}

uint8_t calculate_dest_pos(uint32_t *left, uint32_t *right, uint32_t *dest, uint8_t dir) {
  uint8_t miss=0;
  if(dir) //Ball moving to the left
  {
  	ticksremaining = calculate_keepout(ball_x, ball_y, ball_dx, ball_dy, left, right);
  	*dest = *left;
  	if(minute_changed) miss=1;
  }
  else
  {
  	ticksremaining = calculate_keepout(ball_x, ball_y, ball_dx, ball_dy, right, left);
  	*dest = *right;
  	if(hour_changed) miss=1;
  }
  *dest -= (PADDLE_H_FIXED / 3);
    if(miss)
    {
      if(*dest < (SCREEN_H_FIXED/3)) {
      	*dest += (PADDLE_H_FIXED*2);
      }
      else if (*dest > ((SCREEN_H_FIXED/3)*2)) {
      	*dest -= (PADDLE_H_FIXED*2);
      }
      else {
      	//*dest = crand(2)?0:SCREEN_H_FIXED;
      	*dest += crand(2) ? (PADDLE_H_FIXED*2) : -(PADDLE_H_FIXED*2);
      }
    }
    return ticksremaining;
}

void step_rat(void) {
  // The keepout is used to know where to -not- put the paddle
  // the 'bouncepos' is where we expect the ball's y-coord to be when
  // it intersects with the paddle area
  static uint8_t right_keepout_top, right_keepout_bot, right_bouncepos, right_endpos;
  static uint8_t left_keepout_top, left_keepout_bot, left_bouncepos, left_endpos;
  static uint32_t dest_paddle_pos;
  static uint32_t right_dest, left_dest;

  // Save old ball location so we can do some vector stuff 
  oldball_x = ball_x;
  oldball_y = ball_y;

  // move ball according to the vector
  ball_x += ball_dx;
  ball_y += ball_dy;
  
    
  
  /************************************* TOP & BOTTOM WALLS */
  // bouncing off bottom wall, reverse direction
  if (ball_y  > (SCREEN_H_FIXED - ball_radius*2*FIXED_MATH - BOTBAR_H_FIXED)) {
    //DEBUG(putstring_nl("bottom wall bounce"));
    ball_y = SCREEN_H_FIXED - ball_radius*2*FIXED_MATH - BOTBAR_H_FIXED;
    ball_dy *= -1;
  }
  
  // bouncing off top wall, reverse direction
  if (ball_y < TOPBAR_H_FIXED) {
    //DEBUG(putstring_nl("top wall bounce"));
    ball_y = TOPBAR_H_FIXED;
    ball_dy *= -1;
  }
  
  
  
  /************************************* LEFT & RIGHT WALLS */
  // the ball hits either wall, the ball resets location & angle
  if (((ball_x/FIXED_MATH)  > (SCREEN_W - ball_radius*2)) || ((int8_t)(ball_x/FIXED_MATH) <= 0) || (!ball_dx && !ball_dy)) {
  if(DEBUGGING) {
    if ((int8_t)(ball_x/FIXED_MATH) <= 0) {
        putstring("Left wall collide");
        if (! minute_changed) {
	  putstring_nl("...on accident");
        } else {
	  putstring_nl("...on purpose");
        }
      } else {
        putstring("Right wall collide");
        if (! hour_changed) {
	  putstring_nl("...on accident");
        } else {
	  putstring_nl("...on purpose");
        }
      }
    }

    // place ball in the middle of the screen
    ball_x = (SCREEN_W_FIXED / 2) - FIXED_MATH;
    ball_y = (SCREEN_H_FIXED / 2) - FIXED_MATH;

    int8_t angle = random_angle();
    ball_dx = MAX_BALL_SPEED;
    ball_dy = MAX_BALL_SPEED;
    ball_dx *= cosine(angle);
    ball_dy *= sine(angle);
    ball_dx /= 0x7FFF;
    ball_dy /= 0x7FFF;
    

    glcdFillRectangle(LEFTPADDLE_X, left_keepout_top, PADDLE_W, left_keepout_bot - left_keepout_top, 0);
    glcdFillRectangle(RIGHTPADDLE_X, right_keepout_top, PADDLE_W, right_keepout_bot - right_keepout_top, 0);

    right_keepout_top = right_keepout_bot = 0;
    left_keepout_top = left_keepout_bot = 0;
    redraw_time_rat = 1;
    minute_changed = hour_changed = 0;
    ticksremaining = calculate_dest_pos(&left_dest, &right_dest, &dest_paddle_pos, ball_dx > 0);

		//left_score = time_h;
		//right_score = time_m;
		setscore_rat();
	}

 

  // save old paddle position
  oldleftpaddle_y = leftpaddle_y;
  oldrightpaddle_y = rightpaddle_y;
  
  
 /* if(ball_dx > 0) {
  // For debugging, print the ball location
  DEBUG(putstring("ball @ (")); 
  DEBUG(uart_putw_dec(ball_x)); 
  DEBUG(putstring(", ")); 
  DEBUG(uart_putw_dec(ball_y)); 
  DEBUG(putstring(")"));
  DEBUG(putstring(" ball_dx @ ("));
  DEBUG(uart_putw_dec(ball_dx));
  DEBUG(putstring(")"));
  DEBUG(putstring(" ball_dy @ ("));
  DEBUG(uart_putw_dec(ball_dy));
  DEBUG(putstring(")"));
  DEBUG(putstring(" ball_dy @ ("));
  DEBUG(uart_putw_dec(ball_dy));
  DEBUG(putstring(")"));
  
  }*/

  /*if(!minute_changed) {
    if((ball_dx < 0) && (ball_x < (SCREEN_W_FIXED/2))) {
    	move_paddle(&leftpaddle_y, ball_y);
    }
  } else {
    //Minute changed.  We now have to miss the ball on purpose, if at all possible.
    //If we don't succeed this time around, we will try again next time around.
    if((ball_dx < 0) && (ball_x < (SCREEN_W_FIXED/2))) {
    	move_paddle(&leftpaddle_y, dest_paddle_pos);
    }
  }*/
  
  //ticksremaining--;
  if((ball_dx < 0) && (ball_x < (SCREEN_W_FIXED/2))) {
    move_paddle(&leftpaddle_y, minute_changed?dest_paddle_pos:(ball_y-(PADDLE_H_FIXED/3)));
  } else if((ball_dx > 0) && (ball_x > (SCREEN_W_FIXED/2))) {
  	move_paddle(&rightpaddle_y, hour_changed?dest_paddle_pos:(ball_y-(PADDLE_H_FIXED/3)));
  } else {
  	if(ball_dx < 0)
  		ticksremaining = calculate_dest_pos(&left_dest, &right_dest, &dest_paddle_pos, 1);
  	else
  		ticksremaining = calculate_dest_pos(&left_dest, &right_dest, &dest_paddle_pos, 0);
  }

  // make sure the paddles dont hit the top or bottom
  if (leftpaddle_y < TOPBAR_H_FIXED +1)
    leftpaddle_y = TOPBAR_H_FIXED + 1;
  if (rightpaddle_y < TOPBAR_H_FIXED + 1)
    rightpaddle_y = TOPBAR_H_FIXED + 1;
  
  if (leftpaddle_y > (SCREEN_H_FIXED - PADDLE_H_FIXED - BOTBAR_H_FIXED - 1))
    leftpaddle_y = (SCREEN_H_FIXED - PADDLE_H_FIXED - BOTBAR_H_FIXED - 1);
  if (rightpaddle_y > (SCREEN_H_FIXED - PADDLE_H_FIXED - BOTBAR_H_FIXED - 1))
    rightpaddle_y = (SCREEN_H_FIXED - PADDLE_H_FIXED - BOTBAR_H_FIXED - 1);
  
  if ((ball_dx > 0) && intersectrect(ball_x/FIXED_MATH, ball_y/FIXED_MATH, ball_radius*2, ball_radius*2, RIGHTPADDLE_X, rightpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H)) {
    ball_dx *= -1;
    ball_x = RIGHTPADDLE_X_FIXED - (ball_radius*2*FIXED_MATH);
    //ball_y = right_dest;
    //ticksremaining = calculate_dest_pos(&left_dest, &right_dest, &dest_paddle_pos, 1);
  }
  if ((ball_dx < 0) && intersectrect(ball_x/FIXED_MATH, ball_y/FIXED_MATH, ball_radius*2, ball_radius*2, LEFTPADDLE_X, leftpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H)) {
    ball_dx *= -1;
    ball_x = LEFTPADDLE_X_FIXED + PADDLE_W_FIXED;
    //ball_y = left_dest;
    //ticksremaining = calculate_dest_pos(&left_dest, &right_dest, &dest_paddle_pos, 0);
  }
  
}

void drawmidline(uint8_t inverted) {
  uint8_t i;
  for (i=0; i < (SCREEN_H/8 - 1); i++) { 
    glcdSetAddress((SCREEN_W-MIDLINE_W)/2, i);
    if (inverted) {
      glcdDataWrite(0xF0);
    } else {
      glcdDataWrite(0x0F);  
    }
  }
  glcdSetAddress((SCREEN_W-MIDLINE_W)/2, i);
  if (inverted) {
    glcdDataWrite(0x20);  
  } else {
    glcdDataWrite(0xCF);  
  }
}

void drawdisplay_rat(uint8_t inverted) {

	setscore_rat();
    // erase old ball
    glcdFillRectangle(oldball_x/FIXED_MATH, oldball_y/FIXED_MATH, ball_radius*2, ball_radius*2, inverted);
    // draw new ball
    glcdFillRectangle(ball_x/FIXED_MATH, ball_y/FIXED_MATH, ball_radius*2, ball_radius*2, ! inverted);

    // draw middle lines around where the ball may have intersected it?
    if  (intersectrect(oldball_x/FIXED_MATH, oldball_y/FIXED_MATH, ball_radius*2, ball_radius*2,
		       SCREEN_W/2-MIDLINE_W, 0, MIDLINE_W, SCREEN_H)) {
      // redraw it since we had an intersection
      drawmidline(inverted);
    }



    
    if (oldleftpaddle_y != leftpaddle_y) {
      // clear left paddle
      glcdFillRectangle(LEFTPADDLE_X, oldleftpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H, inverted);
    }
      // draw left paddle
      glcdFillRectangle(LEFTPADDLE_X, leftpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H, !inverted);
    

    if (oldrightpaddle_y != rightpaddle_y) {
      // clear right paddle
      glcdFillRectangle(RIGHTPADDLE_X, oldrightpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H, inverted);
    }
      // draw right paddle
      glcdFillRectangle(RIGHTPADDLE_X, rightpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H, !inverted);
    

    if (intersectrect(oldball_x/FIXED_MATH, oldball_y/FIXED_MATH, ball_radius*2, ball_radius*2, RIGHTPADDLE_X, rightpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H)) {
      glcdFillRectangle(RIGHTPADDLE_X, rightpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H, !inverted);
    }
    if (intersectrect(oldball_x/FIXED_MATH, oldball_y/FIXED_MATH, ball_radius*2, ball_radius*2, LEFTPADDLE_X, leftpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H)) {
      glcdFillRectangle(LEFTPADDLE_X, leftpaddle_y/FIXED_MATH, PADDLE_W, PADDLE_H, !inverted);
    }
   // draw time
   uint8_t redraw_digits;
   TIMSK2 = 0;	//Disable Timer 2 interrupt, to prevent a race condition.
   if(redraw_time_rat)
   {
   	   redraw_digits = 1;
   	   redraw_time_rat = 0;
   }
   TIMSK2 = _BV(TOIE2); //Race issue gone, renable.
    
    draw_score_rat(redraw_digits,inverted);
    
    redraw_digits = 0;
    
}

uint8_t intersectrect(uint8_t x1, uint8_t y1, uint8_t w1, uint8_t h1,
		      uint8_t x2, uint8_t y2, uint8_t w2, uint8_t h2) {
  // yer everyday intersection tester
  // check x coord first
  if (x1+w1 < x2)
    return 0;
  if (x2+w2 < x1)
    return 0;

  // check the y coord second
  if (y1+h1 < y2)
    return 0;
  if (y2+h2 < y1)
    return 0;

  return 1;
}

// 8 pixels high
extern unsigned char BigFont[];

#ifdef OPTION_DOW_DATELONG
uint8_t rat_time_loc[8] = {
	DISPLAY_H10_X_RAT,
	DISPLAY_H1_X_RAT,
	DISPLAY_M10_X_RAT,
	DISPLAY_M1_X_RAT,
	DISPLAY_MON1_X,
	DISPLAY_DOW1_X,
	DISPLAY_DOW2_X,
	DISPLAY_DOW3_X
};
#endif

void check_ball_digit_collision(uint8_t redraw_digits, uint8_t digit_x, uint8_t digit, uint8_t inverted)
{
	if (redraw_digits || intersectrect(oldball_x/FIXED_MATH, oldball_y/FIXED_MATH, ball_radius*2, ball_radius*2,
					digit_x, DISPLAY_TIME_Y_RAT, DISPLAY_DIGITW, DISPLAY_DIGITH)) {
#ifdef OPTION_DOW_DATELONG
        if(digit > 10)
          drawbigfont(digit_x, DISPLAY_TIME_Y_RAT, digit, inverted);
        else
#endif
	      drawbigdigit(digit_x, DISPLAY_TIME_Y_RAT, digit, inverted);
      }
}

void draw_score_rat(uint8_t redraw_digits, uint8_t inverted) {
#ifdef OPTION_DOW_DATELONG
	uint8_t i;
    static uint8_t prev_mode;
	if(score_mode==SCORE_MODE_DOW) {
		if(prev_mode != SCORE_MODE_DOW)
		{
			for(i=0;i<4;i++)
				drawbigdigit(rat_time_loc[i],DISPLAY_TIME_Y_RAT, 10, inverted);
			glcdFillRectangle(ball_x/FIXED_MATH, ball_y/FIXED_MATH, ball_radius*2, ball_radius*2, ! inverted);
			prev_mode = SCORE_MODE_DOW;
		}
		
		for(i=0;i<3;i++)
			check_ball_digit_collision(redraw_digits, rat_time_loc[i+5], sdotw(dotw(date_m,date_d,date_y),i), inverted); 
	}
	else if (score_mode==SCORE_MODE_DATELONG) {
		if(prev_mode != SCORE_MODE_DATELONG)
		{
			if(prev_mode == SCORE_MODE_DOW) {
			for(i=0;i<3;i++)
			  drawbigfont(rat_time_loc[i+5], DISPLAY_TIME_Y_RAT, ' ', inverted);
		    }
		    if(prev_mode == SCORE_MODE_TIME) {
		    for(i=0;i<4;i++)
		      drawbigdigit(rat_time_loc[i], DISPLAY_TIME_Y_RAT, 10, inverted);
		    }
			glcdFillRectangle(ball_x/FIXED_MATH, ball_y/FIXED_MATH, ball_radius*2, ball_radius*2, ! inverted);
			prev_mode = SCORE_MODE_DATELONG;
		}
		for(i=0;i<3;i++)
			check_ball_digit_collision(redraw_digits, rat_time_loc[i+4], smon(date_m,i), inverted); 
		check_ball_digit_collision(redraw_digits, DISPLAY_DAY10_X, right_score/10, inverted);
		check_ball_digit_collision(redraw_digits, DISPLAY_DAY10_X, right_score%10, inverted);
	}
	else {
	  if((prev_mode == SCORE_MODE_DOW) || (prev_mode == SCORE_MODE_DATELONG))
		{
			if(prev_mode == SCORE_MODE_DATELONG) {
			  for(i=0;i<3;i++)
			    drawbigfont(rat_time_loc[i+4], DISPLAY_TIME_Y_RAT, ' ', inverted);
			  drawbigdigit(DISPLAY_DAY10_X, DISPLAY_TIME_Y_RAT, 10, inverted);
			  drawbigdigit(DISPLAY_DAY1_X, DISPLAY_TIME_Y_RAT, 10, inverted);
			}
			if(prev_mode == SCORE_MODE_DOW) {
			  for(i=0;i<3;i++)
			    drawbigfont(rat_time_loc[i+5], DISPLAY_TIME_Y_RAT, ' ', inverted);
		    }
		    if(prev_mode == SCORE_MODE_TIME) {
		      for(i=0;i<4;i++)
			    drawbigdigit(rat_time_loc[i], DISPLAY_TIME_Y_RAT, 10, inverted);
		    }
			glcdFillRectangle(ball_x/FIXED_MATH, ball_y/FIXED_MATH, ball_radius*2, ball_radius*2, ! inverted);
			prev_mode = SCORE_MODE_TIME;
	    }
#endif
	check_ball_digit_collision(redraw_digits, DISPLAY_H10_X_RAT,left_score/10,inverted);
	check_ball_digit_collision(redraw_digits, DISPLAY_H1_X_RAT,left_score%10,inverted);
	check_ball_digit_collision(redraw_digits, DISPLAY_M10_X_RAT,right_score/10,inverted);
	check_ball_digit_collision(redraw_digits, DISPLAY_M1_X_RAT,right_score%10,inverted);
#ifdef OPTION_DOW_DATELONG
  }
#endif
}


void drawbigdigit(uint8_t x, uint8_t y, uint8_t n, uint8_t inverted) {
  uint8_t i, j;
  
  for (i = 0; i < 4; i++) {
    uint8_t d = eeprom_read_byte(&BigFont[(n*4)+i]);
    for (j=0; j<8; j++) {
      if (d & _BV(7-j)) {
	glcdFillRectangle(x+i*2, y+j*2, 2, 2, !inverted);
      } else {
	glcdFillRectangle(x+i*2, y+j*2, 2, 2, inverted);
      }
    }
  }
}

#ifdef OPTION_DOW_DATELONG
//uint8_t get_font(uint16_t addr)
void drawbigfont(uint8_t x, uint8_t y, uint8_t n, uint8_t inverted) {
  uint8_t i, j;
  
  for (i = 0; i < 5; i++) {
    uint8_t d = get_font(((n-0x20)*5)+i);
    for (j=0; j<7; j++) {
      if (d & _BV(j)) {
	glcdFillRectangle(x+i*2, y+j*2, 2, 2, !inverted);
      } else {
	glcdFillRectangle(x+i*2, y+j*2, 2, 2, inverted);
      }
    }
  }
}
#endif

int8_t random_angle(void) {
	uint32_t angle = crand(0);
	angle *= (64 - MIN_BALL_ANGLE*2);
	angle /= RAND_MAX;
	angle += MIN_BALL_ANGLE;
	uint8_t quadrant = (crand(1)) % 4;
	angle += quadrant*64;
	return angle & 0xFF;
}


uint8_t calculate_keepout(int32_t theball_x, int32_t theball_y, int32_t theball_dx, int32_t theball_dy, uint32_t *keepout1, uint32_t *keepout2)
{
  // "simulate" the ball bounce...its not optimized (yet)
  int32_t sim_ball_y = theball_y;
  int32_t sim_ball_x = theball_x;
  int32_t sim_ball_dy = theball_dy;
  int32_t sim_ball_dx = theball_dx;
  
  uint8_t tix = 0, collided = 0;

  while ((sim_ball_x < (RIGHTPADDLE_X_FIXED + PADDLE_W_FIXED)) && ((sim_ball_x + (ball_radius*2*FIXED_MATH)) > LEFTPADDLE_X_FIXED)) {
    int32_t old_sim_ball_x = sim_ball_x;
    int32_t old_sim_ball_y = sim_ball_y;
    sim_ball_y += sim_ball_dy;
    sim_ball_x += sim_ball_dx;
	
    if (sim_ball_y  > (SCREEN_H_FIXED - ball_radius*2*FIXED_MATH - BOTBAR_H_FIXED)) {
      sim_ball_y = SCREEN_H_FIXED - ball_radius*2*FIXED_MATH - BOTBAR_H_FIXED;
      sim_ball_dy *= -1;
    }
	
    if (sim_ball_y <  TOPBAR_H_FIXED) {
      sim_ball_y = TOPBAR_H_FIXED;
      sim_ball_dy *= -1;
    }
    
    if (((sim_ball_x + ball_radius*2*FIXED_MATH) >= RIGHTPADDLE_X_FIXED) && 
	((old_sim_ball_x + ball_radius*2*FIXED_MATH) < RIGHTPADDLE_X_FIXED)) {
      // check if we collided with the right paddle
      
      // first determine the exact position at which it would collide
      int32_t dx = RIGHTPADDLE_X_FIXED - (old_sim_ball_x + ball_radius*2*FIXED_MATH);
      // now figure out what fraction that is of the motion and multiply that by the dy
      int32_t dy = (dx / sim_ball_dx) * sim_ball_dy;
	  
      //if(DEBUGGING){putstring("RCOLL@ ("); uart_putw_dec(old_sim_ball_x + dx); putstring(", "); uart_putw_dec(old_sim_ball_y + dy);}
      
      *keepout1 = (old_sim_ball_y + dy); 
      collided = 1;
    } else if ((sim_ball_x <= (LEFTPADDLE_X_FIXED + PADDLE_W_FIXED)) && 
			(old_sim_ball_x > (LEFTPADDLE_X_FIXED + PADDLE_W_FIXED))) {
      // check if we collided with the left paddle

      // first determine the exact position at which it would collide
      int32_t dx = (LEFTPADDLE_X_FIXED + PADDLE_W_FIXED) - old_sim_ball_x;
      // now figure out what fraction that is of the motion and multiply that by the dy
      int32_t dy = (dx / sim_ball_dx) * sim_ball_dy;
	  
      //if(DEBUGGING){putstring("LCOLL@ ("); uart_putw_dec(old_sim_ball_x + dx); putstring(", "); uart_putw_dec(old_sim_ball_y + dy);}
      
      *keepout1 = (old_sim_ball_y + dy); 
      collided = 1;
    }
    if (!collided) {
      tix++;
    }
    
    //if(DEBUGGING){putstring("\tSIMball @ ["); uart_putw_dec(sim_ball_x); putstring(", "); uart_putw_dec(sim_ball_y); putstring_nl("]");}
  }
  *keepout2 = sim_ball_y / FIXED_MATH;

  return tix;
}
      
//#ifdef RATTCHRON
#endif