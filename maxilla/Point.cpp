
//============================================================================= 
// Maxilla, an OpenGL-based program for viewing dentistry-related VRML & STL.
// Copyright (C) 2008-2012 by Zack T Smith and Ortho Cast Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
// The author may be reached at fbui@comcast.net.
//============================================================================= 

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "defs.h"
#include "Point.h"

#ifdef WIN32
#include "../../zlib/zlib.h"
#else 
#include <zlib.h>
#endif

// ---- from defs.h ----
#if 0
#ifndef bool
typedef char bool;
enum { true=1, false=0 };
#endif
#endif
extern unsigned long total_allocated;
#define INITIAL_TRIANGLES_ARRAY_SIZE (16)
// ---- ----------- ----


Point* Point_new (double _x, double _y, double _z) 
{
	Point *p = (Point*) malloc (sizeof(Point));
	if (!p)
		fatal ("Out of memory!");
	p->x = _x;
	p->y = _y;
	p->z = _z;
	p->n_normals_from_triangles = 0;
	p->normals_from_triangles = NULL;
	p->normal_x = p->normal_y = p->normal_z = 0.f;
	p->n_normals_added = 0;
	p->valid_vertex_normal = false;
	p->along_crease = false;

	total_allocated += sizeof(Point);
	return p;
}

/*===================================================================
 * Name:	realloc_triangle_normals_array
 * Purpose:	Expands said array.
 */
void Point_realloc_triangle_normals_array (Point* p)
{
	int i;
	if (p->normals_from_triangles == NULL)
		return;

	int new_size = (4 * p->n_normals_from_triangles)/3;
	int sz = sizeof(float) * new_size * 4;
	float *nu = (float*) malloc (sz);
	if (!nu) 
		fatal ("Out of memory!");

	memset(nu,0,sz);
	for (i=0; i < p->n_normals_added; i++)
		nu[i] = p->normals_from_triangles[i];
	
	free (p->normals_from_triangles);
	p->normals_from_triangles = nu;
	p->n_normals_from_triangles = new_size;
printf ("Reallocated Point->normals_from_triangles, new size is %d.\n", new_size);
}

/*===================================================================
 * Name:	destroy_triangle_normals_array
 * Purpose:	Clean up said array.
 */
void Point_destroy_triangle_normals_array (Point *p)
{
	if (p->normals_from_triangles != NULL) {
		free (p->normals_from_triangles);
		p->normals_from_triangles = NULL;
	}
	p->n_normals_from_triangles = 0;
	p->n_normals_added = 0;
}

/*===================================================================
 * Name:	init_triangle_normals_array
 * Purpose:	Set up said array.
 */
void Point_init_triangle_normals_array (Point *p)
{
	int i;
	p->normal_x = p->normal_y = p->normal_z = 0.f;
	p->n_normals_from_triangles = INITIAL_TRIANGLES_ARRAY_SIZE;
	int sz = sizeof(float) * p->n_normals_from_triangles * 4;
	p->normals_from_triangles = (float*) malloc (sz);
	if (!p->normals_from_triangles) 
		fatal ("Out of memory!");

	memset(p->normals_from_triangles, 0, sz);
	for (i=0; i < p->n_normals_from_triangles * 4; i++)
		p->normals_from_triangles[i] = 0.f;
	p->n_normals_added = 0;
}

/*===================================================================
 * Name:	serialize
 * Purpose:	Express the point as ASCII.
 */
void Point_serialize (Point *p, gzFile f) 
{
	if (!f) 
		return;

	gzprintf (f, "%g %g %g\n", p->x, p->y, p->z);
}

/*===================================================================
 * Name:	express
 * Purpose:	Expresses the point as OpenGL calls.
 */
void Point_express (Point *p) 
{
	glVertex3d (p->x,p->y,p->z);
}

/*===================================================================
 * Name:	express_smooth
 * Purpose:	Compute the vertex normal, which is the average of 
 *			all adjacent face normals.
 */
void Point_express_smooth (Point *p)
{
	glNormal3d (p->normal_x, p->normal_y, p->normal_z);
	glVertex3d (p->x,p->y,p->z);
}

/*===================================================================
 * Name:	distance
 * Purpose:	Calculate the distance from this to another point.
 */
float Point_distance (Point *p, Point *p2) 
{
	float a,b,c;
	a = p->x - p2->x;
	b = p->y - p2->y;
	c = p->z - p2->z;
	return sqrt (a*a + b*b + c*c);
}

void Point_free (Point *p)
{
	if (p) {
		Point_destroy_triangle_normals_array (p);
		free (p);
	}
}
