/*=============================================================================
 * Maxilla, an OpenGL-based program for viewing dentistry-related VRML & STL.
 * Copyright (C) 2008-2010 by Zack T Smith and Ortho Cast Inc.
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

#ifdef WIN32
	#include <windows.h>
	#define _USE_MATH_DEFINES
#endif

#include <stdio.h>
#include <math.h>

#include "quat.h"

/*---------------------------------------------------------------------------
 * Name:	Quat::multiply 
 * Purpose:	Multiplies two quaternions, stores the product in this Quat.
 * Note:	This is needed because in order to convert yaw & pitch to a
 *		rotation matrix, one has to first multiple the quats
 *		for yaw, pitch, and roll & convert that quat to the matrix.
 */
void Quat::multiply (const Quat *q1, const Quat *q2)
{
	w = q1->w * q2->w - q1->x * q2->x - q1->y * q2->y - q1->z * q2->z;
	x = q1->w * q2->x + q1->x * q2->w + q1->y * q2->z - q1->z * q2->y;
	y = q1->w * q2->y + q1->y * q2->w + q1->z * q2->x - q1->x * q2->z;
	z = q1->w * q2->z + q1->z * q2->w + q1->x * q2->y - q1->y * q2->x;
}

/*---------------------------------------------------------------------------
 * Name:	Quat::normalize 
 * Purpose:	Normalize this quat.
 */
void Quat::normalize ()
{
	double length = sqrt (x*x + y*y + z*z + w*w);
	x /= length;
	y /= length;
	z /= length;
	w /= length;
}

/*---------------------------------------------------------------------------
 * Name:	Quat::from_euler 
 * Purpose:	Converts yaw & pitch to a quaternion, storing in this Quat.
 * Note:	Yes, I know.
 */
void Quat::from_euler (float yaw_, float pitch, float roll, 
					  bool kludge)
{
	float r, p, yaw;

	if (!kludge) {
		r = ((float)M_PI) * roll / 180.f / 2.f;
		p = ((float)M_PI) * pitch / 180.f / 2.f;
		yaw = ((float)M_PI) * yaw_ / 180.f / 2.f;
	} else {
		yaw = ((float)M_PI) * roll / 180.f / 2.f;
		r = ((float)M_PI) * pitch / 180.f / 2.f;
		p = ((float)M_PI) * yaw_ / 180.f / 2.f;
	}

	w = cosf(r)*cosf(p)*cosf(yaw) + sinf(r)*sinf(p)*sinf(yaw);
	x = sinf(r)*cosf(p)*cosf(yaw) - cosf(r)*sinf(p)*sinf(yaw);
	y = cosf(r)*sinf(p)*cosf(yaw) + sinf(r)*cosf(p)*sinf(yaw);
	z = cosf(r)*cosf(p)*sinf(yaw) - sinf(r)*sinf(p)*cosf(yaw);

	normalize ();
}

/*---------------------------------------------------------------------------
 * Name:	Quat::to_matrix4 
 * Purpose:	Converts a quat to a rotation matrix.
 * Note:	Quaternions are just an intermediate.
 */
void Quat::to_matrix4 (float *m)
{
	normalize ();

	m[0] = 1.f - 2.f * (y*y+z*z);
	m[1] = 2.f * (x*y-z*w);
	m[2] = 2.f * (x*z+y*w);
	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.f;
	m[4] = 2.f * (x*y+z*w);
	m[5] = 1.f - 2.f * (x*x+z*z);
	m[6] = 2.f * (y*z-x*w);
	m[8] = 2.f * (x*z-y*w);
	m[9] = 2.f * (y*z+x*w);
	m[10] = 1.f - 2.f * (x*x+y*y);
	m[15] = 1.f;
}

