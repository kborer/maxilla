
/*=============================================================================
  bmplib, a simple library to create, modify, and write BMP image files.
  Copyright (C) 2009 by Zack T Smith.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2 
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  The author may be reached at fbui@comcast.net.
 *============================================================================*/

#ifndef _BMP_H
#define _BMP_H

typedef struct {
	int width, height;
	unsigned long *pixels;
} BMP;

BMP* BMP_new (int, int);
void BMP_delete (BMP*);
int BMP_write (BMP*, char *path);
void BMP_putpixel (BMP*, int, int, unsigned long);
unsigned long BMP_getpixel (BMP*, int, int);

#define RGB_RED (0xff0000)
#define RGB_GREEN (0xff00)
#define RGB_BLUE (0xff)
#define RGB_WHITE (0xffffff)
#define RGB_YELLOW (0xffff00)

#endif

