/*=============================================================================
 * Maxilla, an OpenGL-based program for viewing dentistry-related VRML & STL.
// Copyright (C) 2008-2010 by Zack T Smith and Ortho Cast Inc.
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

#ifndef _QUAT_H
#define _QUAT_H

class Quat {
public:
	double w, x, y, z;
	Quat () {
		w = x = y = z = 0.;
	}
	
	void normalize ();

	Quat& operator= (Quat &q) {
		w = q.w;
		x = q.x;
		y = q.y;
		z = q.z;
		return *this;
	}

	void multiply (const Quat *, const Quat *);

	void from_euler (float yaw, float pitch, float roll, 
					  bool kludge);
	void to_matrix4 (float *);
};

#endif
