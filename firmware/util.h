#include <avr/pgmspace.h>

// Raw constants for the UART to make the bit timing nice

#if (F_CPU == 16000000)
#define BRRL_4800 206
#define BRRL_9600 103    // for 16MHZ
#define BRRL_192 52    // for 16MHZ
#elif (F_CPU == 8000000)
#define BRRL_4800 103
#define BRRL_9600 52
#define BRRL_192 26    
#endif

// Debug printing functions - handy!
#define uart_putc(c) uart_putchar(c)

int uart_putchar(char c);
void uart_init(uint16_t BRR);
char uart_getchar(void);

void uart_putc_hex(uint8_t b);
void uart_putw_hex(uint16_t w);
void uart_putdw_hex(uint32_t dw);

void uart_putw_dec(uint16_t w);
void uart_putdw_dec(uint32_t dw);
void uart_puts(const char* str);

void RAM_putstring(char *str);
void ROM_putstring(const char *str, uint8_t nl);

// by default we stick strings in ROM to save RAM
#define putstring(x) ROM_putstring(PSTR(x), 0)
#define putstring_nl(x) ROM_putstring(PSTR(x), 1)
#define nop asm volatile ("nop\n\t")

// some timing functions

void delay_ms(uint16_t ms);
void delay_10us(uint8_t us);
void delay_s(uint8_t s);

// Date Time Function
uint8_t dotw(uint8_t mon, uint8_t day, uint8_t yr);
uint8_t sdtow(uint8_t dow, uint8_t ix);
uint8_t smon(uint8_t date_m, uint8_t ix);
uint8_t hours(uint8_t h);

// 8 pixels high

uint8_t intersectrect(uint8_t x1, uint8_t y1, uint8_t w1, uint8_t h1,
		      uint8_t x2, uint8_t y2, uint8_t w2, uint8_t h2);
void drawbigdigit(uint8_t x, uint8_t y, uint8_t n, uint8_t inverted);
void init_crand(void);
uint16_t crand(uint8_t type);


void print_menu_advance(void);
void print_menu_change(void);
void print_menu_exit(void);
void PRINT_MENU_OPTS(const char *Opt1, const char *Opt2);
#define print_menu_opts(a,b) PRINT_MENU_OPTS(PSTR(a),PSTR(b))
void PRINT_MENU(const char*, const char*, const char*, const char*);
#define print_menu(a,b,c,d) PRINT_MENU(PSTR(a),PSTR(b),PSTR(c),PSTR(d))
void PRINT_MENU_LINE(uint8_t line, const char *Button, const char *Opt);
#define print_menu_line(a,b,c) PRINT_MENU_LINE(a,PSTR(b),PSTR(c))

uint8_t check_timeout(void);
void add_month(volatile uint8_t *month, volatile uint8_t *day, uint16_t year);