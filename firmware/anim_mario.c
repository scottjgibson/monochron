/* ***************************************************************************
// anim.c - the main animation and drawing code for MONOCHRON
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
**************************************************************************** */

// MarioChron: Mario Themed Clock Face
// Initial work based on InvaderChron by Dataman
// By: techninja (James T) & Super-Awesome Sylvia
//
// Originally created for Sylvia's Super-Awesome Mini Maker Show Episode S02E03
// sylviashow.com/monochron

// == EASILY HACKABLE DEFINES ==================================================

#define GROUND_Y 58    // Ground Position

#define CLOUD_X 65     // Cloud Position
#define CLOUD_Y 11

#define BOX_X 50       // Box Position
#define BOX_Y 26
#define BOX_TYPE 1     // Box Type (0 for standard, 1 for question)

#define TURTLE_MIN 83  // Turtle boundary
#define TURTLE_MAX 110
#define TURTLE_Y 27    // TODO: Derive these from bolted box position
#define TURTLE_SPIN 0  // Shell spin on or off

// ^^ EASILY HACKABLE DEFINES ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t old_m, old_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarming, alarm_h, alarm_m;
extern volatile uint8_t time_format;
extern volatile uint8_t region;
extern volatile uint8_t score_mode;
extern volatile uint8_t write_font;

uint8_t left_score, right_score, left_score2, right_score2;

extern volatile uint8_t minute_changed, hour_changed;

/**** FUNCTION PROTOTYPES ****/
//void encipher(void);
//void init_crand(void);

void WriteDigits(uint8_t, uint8_t);
void WriteTime(uint8_t);
void initdisplay_mar(uint8_t inverted); // Draw the clock face initially (and static objects)
void setscore(void);  // TODO: Document these functions!
void initanim_mar(void);
void animation_loop(void); // Runs all animation routines with a delay
void animate_mario(void);
void animate_turtle(void);
uint8_t animate_showcoin(u08 x, u08 y);

// All draw_* functions draw sprites to the screen.
// Some remove previously drawn frames, some don't.
void draw_showcoin(u08 x, u08 y, u08 anim_frame); // TODO: Add 'inverted' support to these
void draw_mario(u08 x, u08 y, u08 flipped);
void draw_turtle(u08 x, u08 y, u08 flipped);
void draw_ground(void);
void draw_bolt(u08 x, u08 y);
void draw_coin(u08 x, u08 y);
void draw_box(u08 x, u08 y, u08 type);
void draw_cloud(u08 x, u08 y);
// ^ More TODO: Optimize these functions to use a single sprite drawing func


uint8_t redraw_time = 0;
uint8_t last_score_mode = 0;

#if 0
uint32_t rval[2]={0,0};
uint32_t key[4];

void encipher(void) {  // Using 32 rounds of XTea encryption as a PRNG.
  unsigned int i;
  uint32_t v0=rval[0], v1=rval[1], sum=0, delta=0x9E3779B9;
  for (i=0; i < 32; i++) {
    v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    sum += delta;
    v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
  }
  rval[0]=v0; rval[1]=v1;
}

void init_crand(void) {
  uint32_t temp;
  key[0]=0x2DE9716E;  //Initial XTEA key. Grabbed from the first 16 bytes
  key[1]=0x993FDDD1;  //of grc.com/password.  1 in 2^128 chance of seeing
  key[2]=0x2A77FB57;  //that key again there.
  key[3]=0xB172E6B0;
  rval[0]=0;
  rval[1]=0;
  encipher();
  temp = alarm_h;
  temp<<=8;
  temp|=time_h;
  temp<<=8;
  temp|=time_m;
  temp<<=8;
  temp|=time_s;
  key[0]^=rval[1]<<1;
  encipher();
  key[1]^=temp<<1;
  encipher();
  key[2]^=temp>>1;
  encipher();
  key[3]^=rval[1]>>1;
  encipher();
  temp = alarm_m;
  temp<<=8;
  temp|=date_m;
  temp<<=8;
  temp|=date_d;
  temp<<=8;
  temp|=date_y;
  key[0]^=temp<<1;
  encipher();
  key[1]^=rval[0]<<1;
  encipher();
  key[2]^=rval[0]>>1;
  encipher();
  key[3]^=temp>>1;
  rval[0]=0;
  rval[1]=0;
  encipher();	//And at this point, the PRNG is now seeded, based on power on/date/time reset.
}

uint16_t crand(uint8_t type) {
  if((type==0)||(type>2))
  {
    wdt_reset();
    encipher();
    return (rval[0]^rval[1])&RAND_MAX;
  } else if (type==1) {
  	return ((rval[0]^rval[1])>>15)&3;
  } else if (type==2) {
  	return ((rval[0]^rval[1])>>17)&1;
  }

  return 0;
}
#endif

void setscore(void){
  if(score_mode != last_score_mode) {
    redraw_time = 1;
    last_score_mode = score_mode;
    // Default left and right displays
    left_score = time_h;
    right_score = time_m;
    if((region == REGION_US)||(region == DOW_REGION_US)) {
      left_score2 = date_m;
      right_score2 = date_d;
    } else {
      left_score2 = date_d;
      right_score2 = date_m;
    }
  }

  switch(score_mode) {
  	case SCORE_MODE_DATELONG:
      right_score2 = date_d;
  	  break;
    case SCORE_MODE_TIME:
      if(alarming && (minute_changed || hour_changed)) {
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
      if((region == REGION_US)||(region == DOW_REGION_US)) {
        left_score = date_m;
        right_score = date_d;
      } else {
        left_score = date_d;
        right_score = date_m;
      }
      break;
    case SCORE_MODE_YEAR:
      left_score2 = 20;
      right_score2 = date_y;
      break;
    case SCORE_MODE_ALARM:
      left_score2 = alarm_h;
      right_score2 = alarm_m;
      break;
  }
  if (time_format == TIME_12H && left_score>12) {left_score = left_score % 12;}
}

void initanim_mar(void) {
  DEBUG(putstring("screen width: "));
  DEBUG(uart_putw_dec(GLCD_XPIXELS));
  DEBUG(putstring("\n\rscreen height: "));
  DEBUG(uart_putw_dec(GLCD_YPIXELS));
  DEBUG(putstring_nl(""));
  initdisplay_mar(0);
}

void initdisplay_mar(uint8_t inverted) {
  // clear screen
  glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);
  // get time & display
  last_score_mode = 99; // ???
  setscore();

  // Write "WORLD" in the top left corner
  glcdSetAddress(0,0);
  write_font = 77;
    // TODO: This is, just wrong. Seriously. Right?
    glcdWriteChar('W', inverted);glcdWriteChar('O', inverted);
    glcdWriteChar('R', inverted);glcdWriteChar('L', inverted);
    glcdWriteChar('D', inverted);
  write_font = 57;

  // Add the coin and draw the time
  draw_coin(80, 0);
  WriteTime(inverted);
  // Draw the bolted box (height doesn't matter as long as its below the ground)
  // TODO: Make a wrapper function for this (should be easy!)
  glcdRectangle(80, 43, 44, 20);
  glcdClearDot(80, 43); glcdClearDot(123, 43); // Round top corners
  draw_bolt(82, 45); draw_bolt(118, 45); // Add top corner bolts

  // Untouchable static elements
  draw_ground();
  draw_cloud(CLOUD_X, CLOUD_Y);

  // Touchable mostly static elements
  draw_box(BOX_X, BOX_Y, BOX_TYPE);
}

void animation_loop(void) {
  // TODO: Measure time taken by animation functions and remove that time from the delay
  animate_mario();
  animate_turtle();

  // TODO: Allow for object overlap without unsetting background pixels

  _delay_ms(18); // ~16 FPS (not guaranteeable unless animation payload is measured)
}

void animate_mario(void){
  static uint8_t x = 0;
  static uint8_t y = 0;
  static uint8_t direction = 0;
  static uint8_t yarc_count = 0;
  static uint8_t run_coin_anim = 0;
  static uint8_t jump_next_chance = 0;
  static uint8_t should_jump = 0;

  // A faked "arc" of Y offsets for the jump. TODO: Calculate this
  uint8_t yarc[] = {3,9,10,12,9,6,2};

  // Maximum X for mario to walk (hits the big bolted box)
  uint8_t max = 72; // TODO: Derive this from actual collision

  // If the minute, or hour has changed, jump the next chance we get
  if(minute_changed || hour_changed){
    minute_changed = 0;
    hour_changed = 0;
    jump_next_chance = 1;
  }

  // Two specific points we should start jumping from (depending on direction)
  // Only flip this on when we're ready to jump next
  if (((direction == 0 && x == BOX_X - 5) || (direction == 1 && x == BOX_X + 5)) && jump_next_chance){
    should_jump = 1;
  }

  y = GROUND_Y - 12; // (ground_pos - sprite height)

  // Jump!
  if (should_jump && jump_next_chance) {
    y = y - yarc[yarc_count];
    yarc_count++;

    // Box hit! (totally not fake and hardcoded, really...)
    if (yarc[yarc_count] == 12){
      run_coin_anim = 1;
    }

    // Stop/reset the jump when we've reached the last value of Y Arc (Hack!)
    if (yarc_count == sizeof(yarc)){
      jump_next_chance = 0;
      should_jump = 0;
      yarc_count = 0;
    }
  }

  if (direction == 0){
    draw_mario(x, y, 0);
    x++;
  }else if (direction == 1){
    draw_mario(x, y, 1);
    x--;
  }

  if (x >= max){ // Switch to going left
    direction = 1;
  }

  if (x <= 0){ // Switch to going right
    direction = 0;
  }

  // Turns itself off when done, Async style!
  if (run_coin_anim){
    run_coin_anim = animate_showcoin(BOX_X, BOX_Y - 10);
  }
}

void animate_turtle(void){
  static uint8_t x = TURTLE_MIN;
  static uint8_t y = TURTLE_Y;
  static uint8_t direction = 0;
  static uint8_t wait_frame = 0;

  if (!wait_frame){

    if (direction == 0){
      draw_turtle(x, y, 0);
      x++;
    }

    if (direction == 1){
      draw_turtle(x, y, 1);
      x--;
    }

    wait_frame = 0;
  }else{
    wait_frame = 0;
  }

  if (x >= TURTLE_MAX){ // Switch to going left
    direction = 1;
  }

  if (x <= TURTLE_MIN){ // Switch to going right
    direction = 0;
  }

}

uint8_t animate_showcoin(u08 x, u08 y){
  static u08 frame = 0;
  static u08 frame_max = 19;
  static u08 box_type = 0;
  draw_showcoin(x, y, frame);

  if (frame == 4){
    redraw_time = 1; // New coin, new time!
  }

  if (frame == frame_max){
    frame = 0;
    if (box_type == 0) {
      draw_box(BOX_X, BOX_Y, 0);
      box_type = 1;
    } 
    else{
      draw_box(BOX_X, BOX_Y, 0);
      box_type = 0;
    }
    return 0;
  }

  frame++;
  return 1;
}

void draw_mario(u08 x, u08 y, u08 flipped){
  uint8_t xp;
  uint8_t yp;
  uint8_t anim_frame;
  static int mario[5][9] = {
    {0x918,0xE9C,0xEC6,0x7CB,0x6C3,0x7D6,0x492,0x500,0x0}, // Moving Right, frame 1
    {0x518,0x49C,0x6C6,0x7CB,0x6C3,0xFD6,0xE92,0x900,0x0}, // Moving Right, frame 2
    {0x500,0x492,0x7D6,0x6C3,0x7CB,0xEC6,0xE9C,0x918,0x0}, // Moving Left, frame 1
    {0x900,0xE92,0xFD6,0x6C3,0x7CB,0x6C6,0x49C,0x518,0x0}, // Moving Left, frame 2
    {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}, // Dummy (?)
  };
  static u08 frame_offset = 0;
  static u08 lastx; static u08 lasty; static u08 lastanim_frame;

  uint8_t sprite_width = 8;
  uint8_t sprite_height = 12;
  int8_t deltax, deltay;

  if (frame_offset == 0){
    frame_offset = 1;
  }else{
    frame_offset = 0;
  }

  if (flipped){
    anim_frame = 2 + frame_offset;
  }else{
    anim_frame = 0 + frame_offset;
  }

  if (lastx || lasty) {
    deltax = x - lastx;
    if ( deltax > 0) {
      glcdFillRectangle(lastx, lasty, deltax, sprite_height, 0);
    }
    else if (deltax < 0) {
      glcdFillRectangle((lastx + sprite_width + deltax), lasty, -deltax, sprite_height, 0);
    }

    deltay = y - lasty;
    if (deltay > 0) {
      glcdFillRectangle(lastx, lasty, sprite_width, deltay, 0);
    }
    else if (deltay < 0) {
      glcdFillRectangle(lastx, lasty+sprite_height+deltay, sprite_width, -deltay, 0);
    }
  }


  lastx = x; lasty = y; lastanim_frame = anim_frame;

  // Draw the sprite
  for(yp = 0; yp < sprite_height; yp++){
    for(xp = 0; xp < sprite_width; xp++){
      if (mario[anim_frame][xp] & (1 << yp)){
        glcdSetDot(x + xp, y + yp);
      }
      else {
        glcdClearDot(x + xp, y + yp);
      }
    }
  }
}

void draw_showcoin(u08 x, u08 y, u08 anim_frame){
  uint8_t xp;
  uint8_t yp;
  static char coin[16][9] = {
    {0x0,0x0,0x0,0xE0,0xE0,0x0,0x0,0x0,0x0}, // Coin show frame 1
    {0x0,0x0,0x0,0x78,0x78,0x0,0x0,0x0,0x0}, // Coin show frame 2
    {0x0,0x0,0x38,0xC6,0xD6,0x38,0x0,0x0,0x0}, // Coin show frame 3
    {0x0,0x38,0x44,0x82,0x92,0x44,0x38,0x0,0x0}, // Coin show frame 4
    {0x3C,0x42,0x81,0x81,0xA1,0x9D,0x42,0x3C,0x0}, // Coin show frame 5
    {0x0,0x3C,0x42,0x81,0xBD,0x42,0x3C,0x0,0x0}, // Coin show frame 6
    {0x0,0x0,0x7E,0x81,0xBD,0x7E,0x0,0x0,0x0}, // Coin show frame 7
    {0x0,0x0,0x0,0xFF,0xFF,0x0,0x0,0x0,0x0}, // Coin show frame 8
    {0x0,0x0,0x0,0x55,0xAA,0x0,0x0,0x0,0x0}, // Coin show frame 9
    {0x0,0x0,0x0,0x3C,0x3C,0x0,0x0,0x0,0x0}, // Coin show frame 10
    {0x0,0x0,0x24,0x18,0x18,0x24,0x0,0x0,0x0}, // Coin show frame 11
    {0x0,0x42,0x24,0x0,0x0,0x24,0x42,0x0,0x0}, // Coin show frame 12
    {0x81,0x42,0x0,0x0,0x0,0x0,0x42,0x81,0x0}, // Coin show frame 13
    {0x81,0x0,0x0,0x0,0x0,0x0,0x0,0x81,0x0}, // Coin show frame 14
    {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},   // Blank for clearing
    {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}, // Dummy (?)
  };
  static u08 lastx; 
  static u08 lasty; 
  static u08 lastanim_frame;
  /* The frames coming in don't match 1:1 with the actual sprite frames (to save repetition)
  0:0
  1:1
  2:2
  3:3
  4:4 - Coin Is fully visible
  5:5 - Starts to rotate
  6:6
  7:7
  8:6 - The deviation starts here, Where the coin rotates again
  9:5
  10:6
  11:7 - Rotation done, start to dissapear
  12:8
  13:9
  14:10
  15:11
  16:12
  17:13
  18:14
  19:15 - Blank frame (a cheap hack to erase the previous frame and draw nothing)
  */

  if (anim_frame > 7){
    switch (anim_frame){
      case 8:
        anim_frame = 6;
        break;
      case 9:
        anim_frame = 5;
        break;
    }

    if (anim_frame > 9){
      anim_frame = anim_frame - 4;
    }
  }


  uint8_t sprite_width = 8;
  uint8_t sprite_height = 16;
  
  // Clear the last sprite
  if (lastx || lasty) {
    for(yp = 0; yp < sprite_height; yp++){
      for(xp = 0; xp < sprite_width; xp++){
        if (coin[lastanim_frame][xp] & (1 << yp)){
          glcdClearDot(lastx + xp, lasty + yp);
        }
      }
    }
  }



  lastx = x; lasty = y; lastanim_frame = anim_frame;

  // Draw the sprite
  for(yp = 0; yp < sprite_height; yp++){
    for(xp = 0; xp < sprite_width; xp++){
      if (coin[anim_frame][xp] & (1 << yp)){
        glcdSetDot(x + xp, y + yp);
      }
    }
  }
}

void draw_turtle(u08 x, u08 y, u08 flipped){
  uint8_t xp;
  uint8_t yp;
  uint8_t anim_frame;
  static int turtle[9][9] = {
    {0x1C00,0xF780,0xFE80,0x9FC0,0x3BFE,0x3FFF,0x2C33,0x78,0x0}, // Moving Right, frame 1
    {0x1C00,0x3780,0x3E80,0x3FC0,0xFBFE,0xFFFF,0x8C33,0x78,0x0}, // Moving Right, frame 2
    {0x78,0x2C33,0x3FFF,0x3BFE,0x9FC0,0xFE80,0xF780,0x1C00,0x0}, // Moving Left, frame 1
    {0x78,0x8C33,0xFFFF,0xFBFE,0x3FC0,0x3E80,0x3780,0x1C00,0x0}, // Moving Left, frame 2
    {0x30,0x6E,0xEB,0xD5,0xD5,0xEB,0x6E,0x30,0x0}, // Shell Spin, frame 1
    {0xF0,0x6E,0x6B,0xD5,0xD5,0xEB,0xEE,0xF0,0x0}, // Shell Spin, frame 2
    {0xF0,0x6E,0x6B,0x75,0x75,0x6B,0x6E,0xF0,0x0}, // Shell Spin, frame 3
    {0xF0,0xEE,0xEB,0xD5,0xD5,0x6B,0x6E,0xF0,0x0}, // Shell Spin, frame 4
    {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}, // Dummy (?)
  };
  static u08 frame_offset = 0;
  static u08 lastx; static u08 lasty; static u08 lastanim_frame;

  int8_t deltax = 0;
  int8_t deltay = 0;
  uint8_t sprite_width = 8;
  uint8_t sprite_height = 16;

  if (!TURTLE_SPIN){
    if (frame_offset == 0){
      frame_offset = 1;
    }else{
      frame_offset = 0;
    }

    if (flipped){
      anim_frame = 2 + frame_offset;
    }else{
      anim_frame = 0 + frame_offset;
    }
  }else{
    if (frame_offset >= 3){
      frame_offset = 0;
    }else{
      frame_offset++;
    }
    y = y + 8; // Offset for the shorter sprite
    anim_frame = 4 + frame_offset;
  }

  if (lastx || lasty) {
    deltax = x - lastx;
    if ( deltax > 0) {
      glcdFillRectangle(lastx, lasty, deltax, sprite_height, 0);
    }
    else if (deltax < 0) {
      glcdFillRectangle((lastx + sprite_width + deltax), lasty, -deltax, sprite_height, 0);
    }

    deltay = y - lasty;
    if (deltay > 0) {
      glcdFillRectangle(lastx, lasty, sprite_width, deltay, 0);
    }
    else if (deltay < 0) {
      glcdFillRectangle(lastx, lasty+sprite_height+deltay, sprite_width, -deltay, 0);
    }
  }

  lastx = x; lasty = y; lastanim_frame = anim_frame;

  // Draw the sprite
  for(yp = 0; yp < sprite_height; yp++){
    for(xp = 0; xp < sprite_width; xp++){
      if (turtle[anim_frame][xp] & (1 << yp)){
        glcdSetDot(x + xp, y + yp);
      }
      else {
        glcdClearDot(x + xp, y + yp);
      }
    }
  }
}

void draw_ground(void){
  static char ground[] = {0xD,0x10,0x9,0x4,0x19,0x20,0x11,0x8,0x0};
  uint8_t y = GROUND_Y;
  uint8_t xp;
  uint8_t yp;
  uint8_t i;

  uint8_t sprite_width = 8;
  uint8_t sprite_height = 6;

  // Draw the sprite
  for(i = 0; i < 16; i++){
    for(yp = 0; yp < sprite_height; yp++){
      for(xp = 0; xp < sprite_width; xp++){
        if (ground[xp] & (1 << yp)){
          glcdSetDot((i * 8) + xp, y + yp);
        }else{
         glcdClearDot((i * 8) + xp, y + yp);
        }
      }
    }
  }
}

void draw_cloud(u08 x, u08 y){
  static int cloud[] = {0x3776,0x6889,0x8441,0x8002,0x6005,0x8001,0x8892,0x776C,0x0};
  uint8_t xp;
  uint8_t yp;

  uint8_t sprite_width = 16;
  uint8_t sprite_height = 8;

  // Draw the sprite
  for(yp = 0; yp < sprite_height; yp++){
    for(xp = 0; xp < sprite_width; xp++){
    // Sprite is stored on it's side (8x16), so draw it flipped
      if (cloud[yp] & (1 << xp)){
        glcdSetDot(x + xp, y + yp);
      }
    }
  }
}

void draw_bolt(u08 x, u08 y){
  static char bolt[] = {0x6,0xD,0xB,0x6,0x0,0x0,0x0,0x0,0x0};
  uint8_t xp;
  uint8_t yp;

  uint8_t sprite_width = 4;
  uint8_t sprite_height = 4;

  // Draw the sprite
  for(yp = 0; yp < sprite_height; yp++){
    for(xp = 0; xp < sprite_width; xp++){
      if (bolt[xp] & (1 << yp)){
        glcdSetDot(x + xp, y + yp);
      }
    }
  }
}

void draw_coin(u08 x, u08 y){
  static char coin[] = {0x3C,0x42,0x81,0x81,0xA1,0x9D,0x42,0x3C,0x0};
  uint8_t xp;
  uint8_t yp;

  uint8_t sprite_width = 8;
  uint8_t sprite_height = 8;

  // Draw the sprite
  for(yp = 0; yp < sprite_height; yp++){
    for(xp = 0; xp < sprite_width; xp++){
      if (coin[xp] & (1 << yp)){
        glcdSetDot(x + xp, y + yp);
      }
    }
  }
}

void draw_box(u08 x, u08 y, u08 type){
  static char boxes[3][9] = {
    //{0x7E,0xD5,0xAB,0xD5,0xAB,0xD5,0xAB,0x7E,0x0}, // Standard
    {0x7E,0x81,0x85,0xD3,0xDB,0x8D,0x81,0x7E,0x0}, // Question
    {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}, // Dummy (?)
  };

  uint8_t xp;
  uint8_t yp;

  uint8_t sprite_width = 8;
  uint8_t sprite_height = 8;

  // Draw the sprite
  for(yp = 0; yp < sprite_height; yp++){
    for(xp = 0; xp < sprite_width; xp++){
      if (boxes[type][xp] & (1 << yp)){
        glcdSetDot(x + xp, y + yp);
      }
    }
  }
}

void WriteTime(uint8_t inverted) {
  write_font = 77;

  // Draw Date
  glcdSetAddress(1,1);
  WriteDigits(left_score2,inverted);
  glcdWriteChar('-',inverted);
  WriteDigits(right_score2,inverted);

  // Draw Time
  glcdSetAddress(89,0);
  glcdWriteChar('x', inverted);
  WriteDigits(left_score,inverted);
  WriteDigits(right_score,inverted);

  write_font = 57;
}

void WriteDigits(uint8_t t, uint8_t inverted)
{
	glcdWriteChar(48 + (t/10),inverted);
	glcdWriteChar(48 + (t%10),inverted);
}


void drawdisplay_mar(uint8_t inverted) {
  // Animation Loop + delay
  animation_loop();
  setscore();

  if (redraw_time){
    redraw_time = 0;
    WriteTime(inverted);
  }
  return;
}


static unsigned char __attribute__ ((progmem)) MonthText[] = {
	0,0,0,
	'J','A','N',
	'F','E','B',
	'M','A','R',
	'A','P','R',
	'M','A','Y',
	'J','U','N',
	'J','U','L',
	'A','U','G',
	'S','E','P',
	'O','C','T',
	'N','O','V',
	'D','E','C',
};

static unsigned char __attribute__ ((progmem)) DOWText[] = {
	'S','U','N',
	'M','O','N',
	'T','U','E',
	'W','E','D',
	'T','H','U',
	'F','R','I',
	'S','A','T',
};

#if 0
uint8_t dotw(uint8_t mon, uint8_t day, uint8_t yr)
{
  uint16_t month, year;

    // Calculate day of the week
    month = mon;
    year = 2000 + yr;
    if (mon < 3)  {
      month += 12;
      year -= 1;
    }
    return (day + (2 * month) + (6 * (month+1)/10) + year + (year/4) - (year/100) + (year/400) + 1) % 7;
}

#endif





