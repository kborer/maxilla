
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
// Note! Converted this to C while addressing out-of-memory issue on OS/X. -ZS
//============================================================================= 

#ifndef _POINT_H
#define _POINT_H

/*---------------------------------------------------------------------------
 * Name:	Point
 * Purpose:	Encapsulates a point or vector.
 */
typedef struct {
	// The point itself.
	float x, y, z;

	// The original identifier of the point.
	int id;

	// Vertex normal for smoothing.
	bool valid_vertex_normal;

	// True if point is along hard edge.
	bool along_crease;

	//bool part_of_big_triangle; // override smoothing code.
	unsigned short n_normals_added;
	unsigned short n_normals_from_triangles;
	float *normals_from_triangles;
	float normal_x, normal_y, normal_z; 
} Point;

extern Point* Point_new (double,double,double);
extern void Point_free (Point*);
extern void Point_express (Point*);
extern void Point_express_smooth (Point*);
extern float Point_distance (Point*,Point*);
extern void Point_init_triangle_normals_array (Point *p);
extern void Point_realloc_triangle_normals_array (Point* p);
extern void Point_destroy_triangle_normals_array (Point *p);

#endif
