
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BMP.h"

/*===========================================================================
 * Name:	BMP_new
 * Purpose:	Creates new image.
 */
BMP* 
BMP_new (int w, int h)
{
	unsigned long size;
	BMP* nu;
	if (w<1 || h<1)
		return NULL;
	//----------

	if (w & 3) 
		w += 4 - (w & 3);
	if (h & 3) 
		h += 4 - (h & 3);

	nu = (BMP*) malloc (sizeof (BMP));
	if (!nu)
		return NULL;
	memset (nu, 0, sizeof (BMP));
	nu->width = w;
	nu->height = h;
	size = w * h * sizeof (long);
	nu->pixels = (unsigned long*) malloc (size);
	if (!nu->pixels) {
		free (nu);
		return NULL;
	}
	memset (nu->pixels, 0, size);
	return nu;
}

/*===========================================================================
 * Name:	BMP_delete
 * Purpose:	Deallocates image.
 */
void 
BMP_delete (BMP* bmp)
{
	if (!bmp)
		return;
	//----------

	if (bmp->pixels)
		free (bmp->pixels);
	free (bmp);
}

/*===========================================================================
 * Name:	BMP_putpixel
 * Purpose:	Writes pixel into image.
 */
void
BMP_putpixel (BMP *bmp, int x, int y, unsigned long rgb)
{
	if (!bmp || x<0 || y<0)
		return;
	if (x >= bmp->width || y >= bmp->height)
		return;
	if (!bmp->pixels)
		return;
	//----------

	bmp->pixels[y*bmp->width + x] = rgb;
}

/*===========================================================================
 * Name:	BMP_getpixel
 * Purpose:	Reads pixel out of image.
 */
unsigned long
BMP_getpixel (BMP *bmp, int x, int y)
{
	if (!bmp || x<0 || y<0)
		return 0;
	if (x >= bmp->width || y >= bmp->height)
		return 0;
	if (!bmp->pixels)
		return 0;
	//----------

	return bmp->pixels[y*bmp->width + x];
}

/*===========================================================================
 * Name:	BMP_write
 * Purpose:	Writes image to BMP file.
 */
int 
BMP_write (BMP* bmp, char *path)
{
	FILE *f;
#define HDRLEN (54)
	unsigned char h[HDRLEN];
	unsigned long len;
	int i, j;

	if (!bmp || !path)
		return -1;
	//----------

	memset (h, 0, HDRLEN);

	//----------------------------------------
	// Create the file.
	//
	f = fopen (path, "wb");
	if (!f) {
		perror ("fopen");
		return 0;
	}

	//----------------------------------------
	// Prepare header
	//
	len = HDRLEN + 3 * bmp->width * bmp->height;
	h[0] = 'B';
	h[1] = 'M';
	h[2] = len & 0xff;
	h[3] = (len >> 8) & 0xff;
	h[4] = (len >> 16) & 0xff;
	h[5] = (len >> 24) & 0xff;
	h[10] = HDRLEN;
	h[14] = 40;
	h[18] = bmp->width & 0xff;
	h[19] = (bmp->width >> 8) & 0xff;
	h[20] = (bmp->width >> 16) & 0xff;
	h[22] = bmp->height & 0xff;
	h[23] = (bmp->height >> 8) & 0xff;
	h[24] = (bmp->height >> 16) & 0xff;
	h[26] = 1;
	h[28] = 24;
	h[34] = 16;
	h[36] = 0x13; // 2835 pixels/meter
	h[37] = 0x0b;
	h[42] = 0x13; // 2835 pixels/meter
	h[43] = 0x0b;

	//----------------------------------------
	// Write header.
	//
	if (HDRLEN != fwrite (h, 1, HDRLEN, f)) {
		perror ("fwrite");
		fclose (f);
		return 0;
	}

	//----------------------------------------
	// Write pixels.
	// Note that BMP has lower rows first.
	//
	for (j=bmp->height-1; j >= 0; j--) {
		for (i=0; i < bmp->width; i++) {
			unsigned char rgb[3];
			int ix = i + j * bmp->width;
			unsigned long pixel = bmp->pixels[ix];
			rgb[0] = pixel & 0xff;
			rgb[1] = (pixel >> 8) & 0xff;
			rgb[2] = (pixel >> 16) & 0xff;
			if (3 != fwrite (rgb, 1, 3, f)) {
				perror ("fwrite");
				fclose (f);
				return 0;
			}
		}
	}

	fclose (f);
	return 1;
}


