#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "ratt.h"
#include "util.h"

extern volatile uint8_t time_format;

// Creates a 8N1 UART connect
// remember that the BBR is #defined for each F_CPU in util.h
void uart_init(uint16_t BRR) {
  UBRR0 = BRR;               // set baudrate counter

  UCSR0B = _BV(RXEN0) | _BV(TXEN0);
  UCSR0C = _BV(USBS0) | (3<<UCSZ00);
  DDRD |= _BV(1);
  DDRD &= ~_BV(0);
}

// Some basic delays...
void delay_10us(uint8_t ns)
{
  uint8_t i;

  while (ns != 0) {
    ns--;
    for (i=0; i< 30; i++) {
      nop;
    }
  }
}

void delay_ms(uint16_t ms)
{
	uint16_t temp = ms;
	while(temp)
	{
		_delay_ms(10);
		if(temp >= 10)
			temp-=10;
		else
			temp=0;
	}
}

void delay_s(uint8_t s) {
  while (s--) {
    delay_ms(1000);
  }
}

// Some uart functions for debugging help
int uart_putchar(char c)
{
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
  return 0;
}

char uart_getchar(void) {
  while (!(UCSR0A & _BV(RXC0)));
  return UDR0;
}

char uart_getch(void) {
  return (UCSR0A & _BV(RXC0));
}

void ROM_putstring(const char *str, uint8_t nl) {
  uint8_t i;

  for (i=0; pgm_read_byte(&str[i]); i++) {
    uart_putchar(pgm_read_byte(&str[i]));
  }
  if (nl) {
    uart_putchar('\n'); uart_putchar('\r');
  }
}

void uart_puts(const char* str)
{
  while(*str)
    uart_putc(*str++);
}


void uart_putc_hex(uint8_t b)
{
  /* upper nibble */
  if((b >> 4) < 0x0a)
    uart_putc((b >> 4) + '0');
  else
    uart_putc((b >> 4) - 0x0a + 'a');

  /* lower nibble */
  if((b & 0x0f) < 0x0a)
    uart_putc((b & 0x0f) + '0');
  else
    uart_putc((b & 0x0f) - 0x0a + 'a');
}

void uart_putw_hex(uint16_t w)
{
  uart_putc_hex((uint8_t) (w >> 8));
  uart_putc_hex((uint8_t) (w & 0xff));
}

void uart_putdw_hex(uint32_t dw)
{
  uart_putw_hex((uint16_t) (dw >> 16));
  uart_putw_hex((uint16_t) (dw & 0xffff));
}

void uart_putw_dec(uint16_t w)
{
  uint16_t num = 10000;
  uint8_t started = 0;

  while(num > 0)
    {
      uint8_t b = w / num;
      if(b > 0 || started || num == 1)
	{
	  uart_putc('0' + b);
	  started = 1;
	}
      w -= b * num;

      num /= 10;
    }
}

void uart_put_dec(int8_t w)
{
  uint16_t num = 100;
  uint8_t started = 0;

  if (w <0 ) {
    uart_putc('-');
    w *= -1;
  }
  while(num > 0)
    {
      int8_t b = w / num;
      if(b > 0 || started || num == 1)
	{
	  uart_putc('0' + b);
	  started = 1;
	}
      w -= b * num;

      num /= 10;
    }
}

void uart_putdw_dec(uint32_t dw)
{
  uint32_t num = 1000000000;
  uint8_t started = 0;

  while(num > 0)
    {
      uint8_t b = dw / num;
      if(b > 0 || started || num == 1)
	{
	  uart_putc('0' + b);
	  started = 1;
	}
      dw -= b * num;

      num /= 10;
    }
}

#ifdef OPTION_DOW_DATELONG
// Date / Time Routines

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

extern uint8_t DOWText[];
extern uint8_t MonthText[];

uint8_t sdotw(uint8_t dow, uint8_t ix) {
 return eeprom_read_byte(&DOWText[(dow*3) + ix]);
}

uint8_t smon(uint8_t date_m, uint8_t ix) {
 return eeprom_read_byte(&MonthText[(date_m*3) + ix]);
}
#endif
uint8_t hours(uint8_t h)
{
	return (time_format == TIME_12H ? ((h + 23) % 12 + 1) : h);
}


