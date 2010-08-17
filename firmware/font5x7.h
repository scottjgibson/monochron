/*! \file font5x7.h \brief Graphic LCD Font (Ascii Characters). */
//*****************************************************************************
//
// File Name	: 'font5x7.h'
// Title		: Graphic LCD Font (Ascii Charaters)
// Author		: Pascal Stang
// Date			: 10/19/2001
// Revised		: 10/19/2001
// Version		: 0.1
// Target MCU	: Atmel AVR
// Editor Tabs	: 4
//
//*****************************************************************************

#ifndef FONT5X7_H
#define FONT5X7_H

#ifndef WIN32
// AVR specific includes
	#include <avr/eeprom.h>
#endif
// standard ascii 5x7 font
// defines ascii characters 0x20-0x7F (32-127)
extern unsigned char Font5x7[];

#endif
