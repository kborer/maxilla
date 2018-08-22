/*=============================================================================
 * Maxilla, an OpenGL-based program for viewing dentistry-related VRML & STL.
 * Copyright (C) 2010 by Zack T Smith and Ortho Cast Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The author may be reached at fbui@comcast.net.
 *============================================================================*/

#ifdef __linux__

#include <stdio.h>
#include <stdlib.h> // system
#include <string.h>

void
linux_message_box (char *t, char *s)
{
	char cmd [1000];
	sprintf (cmd, "xmessage \"%s\"", s);
	system (cmd);
}

int
linux_file_open (char *path, int len)
{
	int retval = 0;
	return retval;
}

int
linux_file_save (char *path, int len)
{
	int retval = 0;
	return retval;
}

#endif
