#define halt(x)  while (1)

#define DEBUGGING 0
#define DEBUG(x)  if (DEBUGGING) { x; }
#define DEBUGP(x) DEBUG(putstring_nl(x))

// Software options. Uncomment to enable.
//BACKLIGHT_ADJUST - Allows software control of backlight, assuming you mounted your 100ohm resistor in R2'.
#define BACKLIGHT_ADJUST 1

//OPTION_DOW_DATELONG - Allows showing Day of Week, and Longer format Dates, 
//Like " sat","0807","2010", or " aug","  07","2010" or " sat"," aug","  07","2010".
//#define OPTION_DOW_DATELONG 1

// This is a tradeoff between sluggish and too fast to see
#define MAX_BALL_SPEED 5 // note this is in vector arith.
#define ball_radius 2 // in pixels

// If the angle is too shallow or too narrow, the game is boring
#define MIN_BALL_ANGLE 20

// how fast to proceed the animation, note that the redrawing
// takes some time too so you dont want this too small or itll
// 'hiccup' and appear jittery
#define ANIMTICK_MS 75

// Beeep!
#define ALARM_FREQ 4000
#define ALARM_MSONOFF 300
 
#define MAXSNOOZE 600 // 10 minutes

// how many seconds we will wait before turning off menus
#define INACTIVITYTIMEOUT 10 

/*************************** DISPLAY PARAMETERS */

// how many pixels to indent the menu items
#define MENU_INDENT 8

#define DIGIT_W 32
#define HSEGMENT_H 6
#define HSEGMENT_W 18
#define VSEGMENT_H 25
#define VSEGMENT_W 6
#define DIGITSPACING_SEV 4
#define DIGITSPACING_XDA 3
#define DOTRADIUS 4

#define DISPLAY_H10_X_SEV  0
#define DISPLAY_H1_X_SEV  VSEGMENT_W + HSEGMENT_W + 2 + DIGITSPACING_SEV
#define DISPLAY_M10_X_SEV  GLCD_XPIXELS - 2*VSEGMENT_W - 2*HSEGMENT_W - 4 - DIGITSPACING_SEV
#define DISPLAY_M1_X_SEV  GLCD_XPIXELS - VSEGMENT_W - HSEGMENT_W - 2
#define DISPLAY_TIME_Y_SEV 0

#define DISPLAY_H10_X_XDA  0
#define DISPLAY_H1_X_XDA  DIGIT_WIDTH + DIGITSPACING_XDA
#define DISPLAY_M10_X_XDA  GLCD_XPIXELS - 2*DIGIT_WIDTH - DIGITSPACING_XDA
#define DISPLAY_M1_X_XDA  GLCD_XPIXELS - DIGIT_WIDTH
#define DISPLAY_TIME_Y_XDA 0


#define DISPLAY_H10_X_RAT 30
#define DISPLAY_H1_X_RAT 45
#define DISPLAY_M10_X_RAT 70
#define DISPLAY_M1_X_RAT 85
#define DISPLAY_TIME_Y_RAT 4


/* not used
#define ALARMBOX_X 20
#define ALARMBOX_Y 24
#define ALARMBOX_W 80
#define ALARMBOX_H 20
*/

/*************************** PIN DEFINITIONS */
// there's more in KS0108.h for the display

#define ALARM_DDR DDRB
#define ALARM_PIN PINB
#define ALARM_PORT PORTB
#define ALARM 6

#define PIEZO_PORT PORTC
#define PIEZO_PIN  PINC
#define PIEZO_DDR DDRC
#define PIEZO 3


/*************************** ENUMS */

// Whether we are displaying time (99% of the time)
// alarm (for a few sec when alarm switch is flipped)
// date/year (for a few sec when + is pressed)
#define SCORE_MODE_TIME 0
#define SCORE_MODE_DATE 1
#define SCORE_MODE_YEAR 2
#define SCORE_MODE_ALARM 3
#define SCORE_MODE_DOW 4
#define SCORE_MODE_DATELONG_MON 5
#define SCORE_MODE_DATELONG_DAY 6

#define SCORE_MODE_TIMEOUT ((displaystyle == STYLE_XDA)?5:3)

// Constants for how to display time & date
#define REGION_US 0
#define REGION_EU 1
#define DOW_REGION_US 2
#define DOW_REGION_EU 3
#define DATELONG 4
#define DATELONG_DOW 5
#define TIME_12H 0
#define TIME_24H 1

//Contstants for calcualting the Timer2 interrupt return rate.
//Desired rate, is to have the i2ctime read out about 6 times
//a second, and a few other values about once a second.
#define OCR2B_BITSHIFT 0
#define OCR2B_PLUS 1
#define OCR2A_VALUE 16
#define TIMER2_RETURN 80
//#define TIMER2_RETURN (8000000 / (OCR2A_VALUE * 1024 * 6))

// displaymode
#define NONE 99
#define SHOW_TIME 0
#define SHOW_DATE 1
#define SHOW_ALARM 2
#define SET_TIME 3
#define SET_ALARM 4
#define SET_DATE 5
#define SET_BRIGHTNESS 6
#define SET_VOLUME 7
#define SET_REGION 8
#define SHOW_SNOOZE 9
#define SET_SNOOZE 10

#define SET_MONTH 11
#define SET_DAY 12
#define SET_YEAR 13

#define SET_HOUR 101
#define SET_MIN 102
#define SET_SEC 103

#define SET_REG 104

#define SET_BRT 105

#define SET_STYLE 200
#define SET_STL   201
#define STYLE_INT 210
#define STYLE_SEV 211
#define STYLE_RAT 212
#define STYLE_XDA 213
#define STYLE_RANDOM 214
#define STYLE_ROTATE 215

// Undefine to disable GPS
#define GPSENABLE 1

#ifdef GPSENABLE
 #define STYLE_GPS 216
 #define STYLE_ABOUT 217
#else
 #define STYLE_ABOUT 216
#endif


// ROTATEPERIOD is the the wait period, in minutes, between screen rotations.
#define ROTATEPERIOD 15 

//DO NOT set EE_INITIALIZED to 0xFF / 255,  as that is
//the state the eeprom will be in, when totally erased.
#define EE_INITIALIZED 0xC3

/*
#define EE_INIT 0
#define EE_ALARM_HOUR 1
#define EE_ALARM_MIN 2
#define EE_BRIGHT 3
#define EE_VOLUME 4
#define EE_REGION 5
#define EE_TIME_FORMAT 6
#define EE_SNOOZE 7*/

extern uint8_t EE_INIT;
extern uint8_t EE_ALARM_HOUR;
extern uint8_t EE_ALARM_MIN;
extern uint8_t EE_BRIGHT;
extern uint8_t EE_REGION;
extern uint8_t EE_TIME_FORMAT;
extern uint8_t EE_SNOOZE;
extern uint8_t EE_STYLE;

/*************************** FUNCTION PROTOTYPES */

uint8_t leapyear(uint16_t y);
void clock_init(void);
void initbuttons(void);
void tick(void);
void setsnooze(void);
void initanim(void);
void initdisplay(uint8_t inverted);
void step(void);
void setscore(void);
void draw(uint8_t inverted);

void set_style(void);
void set_alarm(void);
void set_time(void);
void set_region(void);
void set_date(void);
void set_backlight(void);
void print_timehour(uint8_t h, uint8_t inverted);
void print_alarmhour(uint8_t h, uint8_t inverted);
void display_menu(void);
void drawArrow(uint8_t x, uint8_t y, uint8_t l);
void setalarmstate(void);
void beep(uint16_t freq, uint8_t duration);
void printnumber(uint8_t n, uint8_t inverted);
void print_time(uint8_t hour, uint8_t min, uint8_t sec, uint8_t mode);


float random_angle_rads(void);

void init_crand(void);
uint8_t dotw(uint8_t mon, uint8_t day, uint8_t yr);

uint8_t i2bcd(uint8_t x);

uint8_t readi2ctime(void);

void writei2ctime(uint8_t sec, uint8_t min, uint8_t hr, uint8_t day,
		  uint8_t date, uint8_t mon, uint8_t yr);

void print_date(uint8_t month, uint8_t day, uint8_t year, uint8_t mode);

void draw7seg(uint8_t x, uint8_t y, uint8_t segs, uint8_t inverted);
void drawsegment(uint8_t s, uint8_t x, uint8_t y, uint8_t inverted);
void drawdigit(uint8_t d, uint8_t x, uint8_t y, uint8_t inverted);
void drawvseg(uint8_t x, uint8_t y, uint8_t inverted);
void drawhseg(uint8_t x, uint8_t y, uint8_t inverted);
void drawdot(uint8_t x, uint8_t y, uint8_t inverted);


// RATT SPECIFIC

// How big our screen is in pixels
#define SCREEN_W 128
#define SCREEN_H 64

#define RIGHTPADDLE_X (SCREEN_W - PADDLE_W - 10)
#define LEFTPADDLE_X 10

// Paddle size (in pixels) and max speed for AI
#define PADDLE_H 12
#define PADDLE_W 3

// How thick the top and bottom lines are in pixels
#define BOTBAR_H 2
#define TOPBAR_H 2

// Specs of the middle line
#define MIDLINE_W 1
#define MIDLINE_H (SCREEN_H / 16) // how many 'stipples'

// Max speed for AI
#define MAX_PADDLE_SPEED 5

#define DISPLAY_DOW1_X 35
#define DISPLAY_DOW2_X 50
#define DISPLAY_DOW3_X 70

#define DISPLAY_MON1_X 20
#define DISPLAY_MON2_X 35
#define DISPLAY_MON3_X 50

// how big are the pixels (for math purposes)
#define DISPLAY_DIGITW 10
#define DISPLAY_DIGITH 16

#define DISPLAY_DAY10_X 70
#define DISPLAY_DAY1_X 85

//THIS IS A GUESS!!!! - IS DATELONG_MON BACKWARDS COMPAT?
#define SCORE_MODE_DATELONG 5

// ====================
// XDALICHRON SPECIFIC

#define DIGIT_WIDTH 28
#define DIGIT_HEIGHT 64
#define MAX_STEPS 32




