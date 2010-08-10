/*! \file fontgr.h \brief Graphic LCD Font (Graphic Characters). */
//*****************************************************************************
//
// File Name	: 'fontgr.h'
// Title		: Graphic LCD Font (Graphic Charaters)
// Author		: Pascal Stang
// Date			: 10/19/2001
// Revised		: 10/19/2001
// Version		: 0.1
// Target MCU	: Atmel AVR
// Editor Tabs	: 4
//
//*****************************************************************************

#ifndef FONTGR_H
#define FONTGR_H

#ifndef WIN32
// AVR specific includes
	#include <avr/pgmspace.h>
#endif

static unsigned char __attribute__((section(".eeprom"))) FontGr[] =
{
// format is one character per line:
// length, byte array[length]
//0x0B,0x3E,0x41,0x41,0x41,0x41,0x42,0x42,0x42,0x42,0x3C,0x00,// 0. Folder Icon
//0x06,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF		       	// 1. Solid 6x8 block
	
16, 88, 188, 28, 22, 22, 63, 63, 22, 22, 28, 188, 88, 0, 0, 0, 0, // 0 = Triangle Up
16, 30, 184, 125, 54, 60, 60, 60, 60, 54, 125, 184, 30, 0, 0, 0, 0,// 1 = Square Up
16, 156, 158, 94, 118, 55, 95, 95, 55, 118, 94, 158, 156, 0, 0, 0, 0, // 2 = Circle Up
16, 24, 28, 156, 86, 182, 95, 95, 182, 86, 156, 28, 24, 0, 0, 0, 0, // 3 = Triangle Down
16, 112, 24, 125, 182, 188, 60, 60, 188, 182, 125, 24, 112, 0, 0, 0, 0, // 4 = Square Down
16, 28, 94, 254, 182, 55, 95, 95, 55, 182, 254, 94, 28, 0, 0, 0, 0, // 5 = Circle Down
12, 248, 252, 254, 254, 63, 31, 31, 63, 254, 254, 252, 248  // 6 = Base
};

#endif
