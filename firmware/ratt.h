#define halt(x)  while (1)

#define DEBUGGING 0
#define DEBUG(x)  if (DEBUGGING) { x; }
#define DEBUGP(x) DEBUG(putstring_nl(x))

// Software options. Uncomment to enable.
//BACKLIGHT_ADJUST - Allows software control of backlight, assuming you mounted your 100ohm resistor in R2'.
#define BACKLIGHT_ADJUST 1

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

// Where the HOUR10 HOUR1 MINUTE10 and MINUTE1 digits are
// in pixels

#define DIGIT_PIXEL_SIZE_X 3
#define DIGIT_PIXEL_SIZE_Y 5

#define DISPLAY_H10_X 0 + 5
#define DISPLAY_H1_X 15 + 5
#define DISPLAY_HM_X 30 + 5
#define DISPLAY_M10_X 45 + 5
#define DISPLAY_M1_X 60 + 5
#define DISPLAY_MS_X 75 + 5
#define DISPLAY_S10_X 90 + 5
#define DISPLAY_S1_X 105 + 5

#define DISPLAY_DL1_X 0 + 5
#define DISPLAY_DL2_X 15 + 5
#define DISPLAY_DL3_X 30 + 5
#define DISPLAY_DL4_X 45 + 5
#define DISPLAY_DR1_X 60 + 5
#define DISPLAY_DR2_X 75 + 5
#define DISPLAY_DR3_X 90 + 5
#define DISPLAY_DR4_X 105 + 5



// buffer space from the top
#define DISPLAY_TIME_Y 12

// how big are the pixels (for math purposes)
#define DISPLAY_DIGITW 16
#define DISPLAY_DIGITH 40

// How big our screen is in pixels
#define SCREEN_W 128
#define SCREEN_H 64


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
#define SCORE_MODE_DEATH_TIME 4
#define SCORE_MODE_DEATH_DATE 5
#define SCORE_MODE_DEATH_YEAR 6
#define SCORE_MODE_DEATH_ALARM 7

// Constants for how to display time & date
#define REGION_US 0
#define REGION_EU 1
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
#define SET_DEATHCLOCK_DOB 11
#define SET_DEATHCLOCK_GENDER 12
#define SET_DEATHCLOCK_MODE 13
#define SET_DEATHCLOCK_BMI 14
#define SET_DEATHCLOCK_SMOKER 15

#define SET_MONTH 101
#define SET_DAY 102
#define SET_YEAR 103

#define SET_HOUR 104
#define SET_MIN 105
#define SET_SEC 106

#define SET_REG 107

#define SET_BRT 108

#define SET_DCGENDER 109
#define SET_DCMODE 110
#define SET_BMI_UNIT 111
#define SET_BMI_WT 112
#define SET_BMI_HT 113
#define SET_DCSMOKER 114

//DO NOT set EE_INITIALIZED to 0xFF / 255,  as that is
//the state the eeprom will be in, when totally erased
#define EE_INITIALIZED 0xC4
#define EE_INIT 0
#define EE_ALARM_HOUR 1
#define EE_ALARM_MIN 2
#define EE_BRIGHT 3
#define EE_VOLUME 4
#define EE_REGION 5
#define EE_TIME_FORMAT 6
#define EE_SNOOZE 7
#define EE_DEATHCLOCK_ON 8
#define EE_DOB_MONTH 9 //Death Clock variables are preserved in the event of an extended power outage.
#define EE_DOB_DAY 10
#define EE_DOB_YEAR 11
#define EE_SET_MONTH 12
#define EE_SET_DAY 13
#define EE_SET_YEAR 14
#define EE_GENDER 15
#define EE_DC_MODE 16
#define EE_BMI_UNIT 17
#define EE_BMI_WEIGHT 18
#define EE_BMI_HEIGHT 20
#define EE_SMOKER 22
#define EE_SET_HOUR 23
#define EE_SET_MIN 24
#define EE_SET_SEC 25

/*************************** FUNCTION PROTOTYPES */

/******* config.c ********/
void set_deathclock_dob(void);
void set_deathclock_gender(void);
void set_deathclock_mode(void);
void set_deathclock_bmi(void);
void set_deathclock_smoker(void);
void set_alarm(void);
void set_time(void);
void set_region(void);
void set_date(void);
void set_backlight(void);
void print_timehour(uint8_t h, uint8_t inverted);
void print_alarmhour(uint8_t h, uint8_t inverted);
void display_menu(void);

/******* ratt.c ********/
uint8_t leapyear(uint16_t y);
void clock_init(void);
void initbuttons(void);
void tick(void);
void setsnooze(void);
void drawArrow(uint8_t x, uint8_t y, uint8_t l);
void setalarmstate(void);
void beep(uint16_t freq, uint8_t duration);
void printnumber_1d(uint8_t n, uint8_t inverted);
void printnumber_2d(uint16_t n, uint8_t inverted);
void printnumber_3d(uint16_t n, uint8_t inverted);
void calc_death_date(void);
uint8_t i2bcd(uint8_t x);
uint8_t readi2ctime(void);
void writei2ctime(uint8_t sec, uint8_t min, uint8_t hr, uint8_t day,
		  uint8_t date, uint8_t mon, uint8_t yr);
void load_etd(void);


/******* anim.c ********/
void initanim(void);
void initdisplay(uint8_t inverted);
void step(void);
void setscore(void);
void draw(uint8_t inverted);
void drawbigdigit(uint8_t x, uint8_t y, uint8_t n, uint8_t inverted);
void blitsegs_rom(int16_t x_origin, uint8_t y_origin, PGM_P bitmap_p, uint8_t height, uint8_t inverted);
void render_image (uint8_t image, int16_t x, uint8_t inverted);
uint8_t intersectrect(int16_t x1, uint8_t y1, uint8_t w1, uint8_t h1,
					  uint8_t x2, uint8_t y2, uint8_t w2, uint8_t h2);







#define SKULL 0
#define REAPER 1
#define RIP 2
#define REAPER_TOW_RIP 3
