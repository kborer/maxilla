
/*=============================================================================
  Maxilla, an OpenGL-based 3D program for viewing dentistry-related VRML & STL.
  Copyright (C) 2008-2010 by Zack T Smith and Ortho Cast Inc.

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

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <hpdf.h>

#ifdef WIN32
#include "stdafx.h"
#endif

#include "PDF.h"
#include "BMP.h"

#include <setjmp.h>

jmp_buf jb;

static void
pdf_error (HPDF_STATUS errnum, HPDF_STATUS detail, void *data)
{
	puts ("PDF could not be generated.");
    	longjmp (jb, 1);
}

//-----------------------------------------------------------------------------
// Name:	put_image
// Purpose:	Draws given BMP image, centered at (center_x, center_y).
//-----------------------------------------------------------------------------
static void
put_image (HPDF_Doc pdf, BMP *bmp, float center_x, float center_y, float w, float h,
		int pixel_width, int pixel_height)
{
	float x = center_x - w/2.f;
	float y = center_y - h/2.f;
	int i, j;
	HPDF_Image image;
	int size;
	HPDF_Page page = HPDF_GetCurrentPage (pdf);
	HPDF_BYTE *gray;

	if (pixel_width > bmp->width)
		pixel_width = bmp->width;
	if (pixel_height > bmp->height)
		pixel_height = bmp->height;

	size = pixel_width * pixel_height;
	gray = (HPDF_BYTE*) malloc (size);
	if (!gray) {
		puts ("Out of memory.");
		return;
	}

	memset (gray, 0xff, size);

	for (i = 0; i < pixel_width; i++) {
		for (j = 0; j < pixel_height; j++) {
			unsigned long color = BMP_getpixel (bmp, i, j);
			unsigned long g = color & 255;
			g += 255 & (color >> 8);
			g += 255 & (color >> 16);
			g /= 3;
			gray [i + (pixel_height - 1 - j) * pixel_width] = (HPDF_BYTE) g;
		}
	}

	image = HPDF_LoadRawImageFromMem (pdf, 
		gray,
		pixel_width, pixel_height-4, // due to bug in HPDF...
		HPDF_CS_DEVICE_GRAY, 8);

	printf("In draw_image, x,y=(%g,%g), dims=%gx%g, pixel_w=%d pixel_h=%d\n",
		x, y, w, h, pixel_width,pixel_height);
	HPDF_Page_DrawImage (page, image, 
		x, y, w, h);

	free (gray);
}

static void
put_text (HPDF_Doc pdf, HPDF_Page page, char **lines, int n_lines, 
			int fs, int where)
{
	int i;
	float width = 0.f;
	float bottom_size = 36.f;
	float font_size = (float) fs;
	float line_height = font_size + 1.f;
	float page_margin = 36.f;
	float page_width = 11.f * 72.f;
	float page_height = 8.5f * 72.f;
	float box_margin = 18.f;
	float height = 2.f * box_margin +
		line_height * n_lines; 

	HPDF_Font font = HPDF_GetFont (pdf, "Courier", NULL);
	HPDF_Page_SetFontAndSize (page, font, font_size);

	//--------------------------------------------------
	// Ascertain the maximum width of all the text lines.
	//
	for (i = 0; i < n_lines; i++) {
		float w = HPDF_Page_TextWidth (page, lines[i]);
		if (w > width)
			width = w;
	}
	width += 2.f * box_margin;

	for (i = 0; i < n_lines; i++) {
		float x, y;

		if (where == TOPLEFT) {
			x = page_margin + box_margin;
			y = page_height -
			  (page_margin + box_margin + (i+1.f) * line_height);
		} else {
			int i2 = i % 3;
			x = page_margin + box_margin;
			if (i >= 3)
				x = page_width / 2.f;
			y = page_margin + bottom_size -
			  (box_margin + (i2+1.f) * line_height);
		}

#if 0
		HPDF_Page_SetRGBStroke (page, 1.0, 1.0, 1.0);
		HPDF_Page_SetRGBFill (page, 1.0, 1.0, 1.0);

		HPDF_Page_BeginText (page);
		HPDF_Page_MoveTextPos (page, x, y);
		HPDF_Page_ShowText (page, lines[i]);
		HPDF_Page_EndText (page);

		HPDF_Page_BeginText (page);
		HPDF_Page_MoveTextPos (page, 0.6f+x, 0.6f+y);
		HPDF_Page_ShowText (page, lines[i]);
		HPDF_Page_EndText (page);

		HPDF_Page_BeginText (page);
		HPDF_Page_MoveTextPos (page, 0.6f+x, y);
		HPDF_Page_ShowText (page, lines[i]);
		HPDF_Page_EndText (page);

		HPDF_Page_BeginText (page);
		HPDF_Page_MoveTextPos (page, x, 0.6f+y);
		HPDF_Page_ShowText (page, lines[i]);
		HPDF_Page_EndText (page);
#endif

		HPDF_Page_SetRGBStroke (page, 0.0, 0.0, 0.0);
		HPDF_Page_SetRGBFill (page, 0.0, 0.0, 0.0);

		HPDF_Page_BeginText (page);
		HPDF_Page_MoveTextPos (page, 0.3f + x, 0.3f + y);
		HPDF_Page_ShowText (page, lines[i]);
		HPDF_Page_EndText (page);
	}

	if (where == BOTTOM) {
		HPDF_Page_SetRGBStroke (page, .5, .5, .5);
		HPDF_Page_MoveTo (page, page_margin, page_margin + 36.f);
		HPDF_Page_LineTo (page, page_margin + page_width
				- 2.f * page_margin, page_margin + 36.f);
		HPDF_Page_Stroke (page);
	}
}

static HPDF_Destination dst;
static HPDF_Doc pdf;
static HPDF_Page page;
static float page_aspect;

int
pdf_start ()
{
	if (setjmp (jb)) {
		puts ("setjmp problem.");
		return 0;
	}

	if (!(pdf = HPDF_New ((HPDF_Error_Handler) pdf_error, NULL))) {
		puts ("PDF cannot not be generated.");
		return 0;
	}

	HPDF_SetCompressionMode (pdf, HPDF_COMP_ALL);

	page = HPDF_AddPage (pdf);
	HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_LANDSCAPE);

	dst = HPDF_Page_CreateDestination (page);
	HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page), 1);
	HPDF_SetOpenAction (pdf, dst);

	//HPDF_Page_SetWidth (page, 72. * 8.5);
	//HPDF_Page_SetHeight (page, 72. * 11);

	// Letter size @ 72 dpi with 1/2 inch margins is just 540x720.
	//
	page_aspect = 720.f / 540.f;

	puts ("PDF creation started.");
	return 1;
}

void
pdf_draw_big_image (BMP *bmp)
{
	float aspect = (float) bmp->width;
	float x, y, w, h;

	aspect /= (float) bmp->height;

	//----------------------------------------
	// Enlargen the image to take up the 
	// maximum space on the page while keeping
	// the margins.
	//
printf ("aspect = %g\n", aspect);
printf ("page_aspect = %g\n", page_aspect);
	if (aspect > page_aspect) {
		x = 36.f;
		w = 720.f;
		h = 720.f / aspect;
		y = 36.f + (540.f - h) / 2.f;
	} else {
		y = 36.f;
		h = 540.f;
		w = 540.f * aspect;
		x = 36.f + (720.f - w) / 2.f;
	}
printf ("w = %g\n", w);
printf ("h = %g\n", h);

	put_image (pdf, bmp, 
		(11*72./2.), (8.5*72./2.), w, h, bmp->width, bmp->height);
}


void
pdf_draw_image (BMP *bmp, float cx, float cy, float w, float h,
		int pixel_width, int pixel_height)
{
	put_image (pdf, bmp, cx, cy, w, h, 
			pixel_width, pixel_height);
}


int
pdf_end (char *pdf_path, char **text, int n_lines, int fontsize, int where)
{
	put_text (pdf, page, text, n_lines, fontsize, where);

	HPDF_SaveToFile (pdf, pdf_path);
	HPDF_Free (pdf);

	puts ("PDF creation ended.");
	return 1;
}

