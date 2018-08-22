
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
	#define _USE_MATH_DEFINES
#else
	#include <sys/time.h> 	// gettimeofday
#endif

#include <stdio.h>
#include <string.h>

#include <math.h>

#include "defs.h"

#ifdef WIN32
#include "stdafx.h"
#endif

#include "maxilla.h"

//---------------------------------------------------------------------------
// Name:	stl_parser
// Purpose:	Parser for binary STL files.
//---------------------------------------------------------------------------
IndexedFaceSet *
stl_parser (char *path, Model *m)
{
	if (!path)
		return NULL;
	//----------

	FILE *f = fopen (path, "rb");
	if (!f) {
		perror("fopen");
		return NULL;
	}

	unsigned char header[80];
	if (80 != fread ((char*) header, 1, 80, f)) {
		perror("fread");
		fclose (f);
		return NULL;
	}

	if (!strncmp ((char*) header, "solid", 5)) {
#ifdef WIN32
		MessageBox (0, _T("It's an ASCII STL file. That type is unsupported."), 0, 0);
#else
		warning ("Selected STL file is the ASCII type, which is unsupported.");
#endif
		fclose (f);
		return NULL;
	}

	int n_triangles;
	if (4 != fread ((char*) &n_triangles, 1, 4, f)) {
		perror("fread");
		fclose (f);
		return NULL;
	}
	
	unsigned long amount = (12 * 4 + 2) * n_triangles;
	unsigned char *buffer = (unsigned char*) malloc (amount);
	if (!buffer) {
#ifdef WIN32
		MessageBoxA (0, "Unable to allocate memory.", 0, 0);
#else
		warning ("Out of memory.");
#endif
		return NULL;
	}

	memset(buffer,0,amount);

	if (amount != fread ((void*) buffer, 1, amount, f)) {
		perror("fread");
		fclose (f);
		return NULL;
	}

	fclose (f);

	IndexedFaceSet *ifs = new IndexedFaceSet ();

	printf ("Binary STL has %d triangles.\n", n_triangles);

	int i;
	float v[12];
	int index = 0;
	float max=-1E6f, min=1E6f;

	ifs->maxx = -1e6f;
	ifs->minx = 1e6f;
	ifs->maxy = -1e6f;
	ifs->miny = 1e6f;
	ifs->maxz = -1e6f;
	ifs->minz = 1e6f;

	for (i = 0; i < n_triangles; i++) {
		unsigned short attr_byte_count;

		memcpy (v, &buffer[index], 12 * 4);
		index += 12 * 4;
		memcpy (&attr_byte_count, &buffer[index], 2);
		index += 2;

		if (attr_byte_count != 0) {
			puts ("Nonzero attr_byte_count.");
			delete ifs;
			return NULL;
		}

		Point *p1 = Point_new (v[3], v[4], v[5]);
		Point *p2 = Point_new (v[6], v[7], v[8]);
		Point *p3 = Point_new (v[9], v[10], v[11]);
		
		if (p1->x < ifs->minx)
			ifs->minx = p1->x;
		if (p1->x > ifs->maxx)
			ifs->maxx = p1->x;
		if (p1->y < ifs->miny)
			ifs->miny = p1->y;
		if (p1->y > ifs->maxy)
			ifs->maxy = p1->y;
		if (p1->z < ifs->minz)
			ifs->minz = p1->z;
		if (p1->z > ifs->maxz)
			ifs->maxz = p1->z;

		if (p2->x < ifs->minx)
			ifs->minx = p2->x;
		if (p2->x > ifs->maxx)
			ifs->maxx = p2->x;
		if (p2->y < ifs->miny)
			ifs->miny = p2->y;
		if (p2->y > ifs->maxy)
			ifs->maxy = p2->y;
		if (p2->z < ifs->minz)
			ifs->minz = p2->z;
		if (p2->z > ifs->maxz)
			ifs->maxz = p2->z;

		if (p3->x < ifs->minx)
			ifs->minx = p3->x;
		if (p3->x > ifs->maxx)
			ifs->maxx = p3->x;
		if (p3->y < ifs->miny)
			ifs->miny = p3->y;
		if (p3->y > ifs->maxy)
			ifs->maxy = p3->y;
		if (p3->z < ifs->minz)
			ifs->minz = p3->z;
		if (p3->z > ifs->maxz)
			ifs->maxz = p3->z;

		Point_free (p1);
		Point_free (p2);
		Point_free (p3);
	}

	float x_offset = (ifs->maxx + ifs->minx) / -2.f;
	float y_offset = (ifs->maxy + ifs->miny) / -2.f;
	float z_offset = (ifs->maxz + ifs->minz) / -2.f;
#if 0
	printf ("x min,max = %g %g\n", ifs->minx, ifs->maxx);
	printf ("y min,max = %g %g\n", ifs->miny, ifs->maxy);
	printf ("z min,max = %g %g\n", ifs->minz, ifs->maxz);
#endif
	ifs->maxx = -1e6f;
	ifs->minx = 1e6f;
	ifs->maxy = -1e6f;
	ifs->miny = 1e6f;
	ifs->maxz = -1e6f;
	ifs->minz = 1e6f;

	index = 0;
	for (i = 0; i < n_triangles; i++) {
		unsigned short attr_byte_count;

		memcpy (v, &buffer[index], 12 * 4);
		index += 12 * 4;
		memcpy (&attr_byte_count, &buffer[index], 2);
		index += 2;

		if (attr_byte_count != 0) {
			puts ("Nonzero attr_byte_count.");
			delete ifs;
			return NULL;
		}
#if 0
		printf ("Triangle %d:\n", i);
		printf ("\tNormal %g %g %g\n", v[0], v[1], v[2]);
		printf ("\tPoint A %g %g %g\n", v[3], v[4], v[5]);
		printf ("\tPoint B %g %g %g\n", v[6], v[7], v[8]);
		printf ("\tPoint C %g %g %g\n", v[9], v[10], v[11]);
#endif
		if (ifs->n_points+3 >= ifs->points_size)
			ifs->expand_points();

		Point *p1 = Point_new (v[3]+x_offset, v[4]+y_offset, v[5]+z_offset);
		Point *p2 = Point_new (v[6]+x_offset, v[7]+y_offset, v[8]+z_offset);
		Point *p3 = Point_new (v[9]+x_offset, v[10]+y_offset, v[11]+z_offset);
		
		if (p1->x < ifs->minx)
			ifs->minx = p1->x;
		if (p1->x > ifs->maxx)
			ifs->maxx = p1->x;
		if (p1->y < ifs->miny)
			ifs->miny = p1->y;
		if (p1->y > ifs->maxy)
			ifs->maxy = p1->y;
		if (p1->z < ifs->minz)
			ifs->minz = p1->z;
		if (p1->z > ifs->maxz)
			ifs->maxz = p1->z;

		if (p2->x < ifs->minx)
			ifs->minx = p2->x;
		if (p2->x > ifs->maxx)
			ifs->maxx = p2->x;
		if (p2->y < ifs->miny)
			ifs->miny = p2->y;
		if (p2->y > ifs->maxy)
			ifs->maxy = p2->y;
		if (p2->z < ifs->minz)
			ifs->minz = p2->z;
		if (p2->z > ifs->maxz)
			ifs->maxz = p2->z;

		if (p3->x < ifs->minx)
			ifs->minx = p3->x;
		if (p3->x > ifs->maxx)
			ifs->maxx = p3->x;
		if (p3->y < ifs->miny)
			ifs->miny = p3->y;
		if (p3->y > ifs->maxy)
			ifs->maxy = p3->y;
		if (p3->z < ifs->minz)
			ifs->minz = p3->z;
		if (p3->z > ifs->maxz)
			ifs->maxz = p3->z;

		ifs->points[ifs->n_points++] = p1;
		ifs->points[ifs->n_points++] = p2;
		ifs->points[ifs->n_points++] = p3;
	}

	for (i = 0; i < ifs->n_points; i++) {
		ifs->points[i]->x /= 1000.f;
		ifs->points[i]->y /= 1000.f;
		ifs->points[i]->z /= 1000.f;
	}

	#if 0
	printf ("x min,max = %g %g\n", ifs->minx, ifs->maxx);
	printf ("y min,max = %g %g\n", ifs->miny, ifs->maxy);
	printf ("z min,max = %g %g\n", ifs->minz, ifs->maxz);
#endif

	ifs->maxx = -1e6f;
	ifs->minx = 1e6f;
	ifs->maxy = -1e6f;
	ifs->miny = 1e6f;
	ifs->maxz = -1e6f;
	ifs->minz = 1e6f;

	for (i = 0; i < ifs->n_points; i += 3) {
		Point *p1 = ifs->points [i];
		Point *p2 = ifs->points [i+1];
		Point *p3 = ifs->points [i+2];
		
		Triangle *t = new Triangle (p1, p2, p3);
		t->model = m;

		if (p1->x < ifs->minx)
			ifs->minx = p1->x;
		if (p1->x > ifs->maxx)
			ifs->maxx = p1->x;
		if (p1->y < ifs->miny)
			ifs->miny = p1->y;
		if (p1->y > ifs->maxy)
			ifs->maxy = p1->y;
		if (p1->z < ifs->minz)
			ifs->minz = p1->z;
		if (p1->z > ifs->maxz)
			ifs->maxz = p1->z;

		if (p2->x < ifs->minx)
			ifs->minx = p2->x;
		if (p2->x > ifs->maxx)
			ifs->maxx = p2->x;
		if (p2->y < ifs->miny)
			ifs->miny = p2->y;
		if (p2->y > ifs->maxy)
			ifs->maxy = p2->y;
		if (p2->z < ifs->minz)
			ifs->minz = p2->z;
		if (p2->z > ifs->maxz)
			ifs->maxz = p2->z;

		if (p3->x < ifs->minx)
			ifs->minx = p3->x;
		if (p3->x > ifs->maxx)
			ifs->maxx = p3->x;
		if (p3->y < ifs->miny)
			ifs->miny = p3->y;
		if (p3->y > ifs->maxy)
			ifs->maxy = p3->y;
		if (p3->z < ifs->minz)
			ifs->minz = p3->z;
		if (p3->z > ifs->maxz)
			ifs->maxz = p3->z;

		if (ifs->n_triangles+1 >= ifs->triangles_size)
			ifs->expand_triangles();
		ifs->triangles[ifs->n_triangles++] = t;
	}

#if 0
	// Test triangle
	Point *p1 = new Point (0.f,0.f,0.f);
	Point *p2 = new Point (1.f,0.f,0.f);
	Point *p3 = new Point (0.f,1.f,0.f);
		Triangle *t = new Triangle (p1, p2, p3);
		ifs->triangles[ifs->n_triangles++] = t;
#endif

	puts ("Done reading binary STL file.");

	free (buffer);

	return ifs;
}

Model *
stl_parser_one_file (char *path)
{
	Model *m;
	m = new Model();

	IndexedFaceSet *ifs = stl_parser (path, m);
	if (!ifs)
		return NULL;

	Node *node;
	node = new Node ();
	m->nodes = node;

	Shape *shape = new Shape ();
	Appearance *appearance = new Appearance ();
	Material *material = new Material ();
	appearance->add_child (material);
	shape->appearance = appearance;

	node->add_child (shape);

	shape->add_geometry (ifs);
	return m;
}

Model *
stl_parser_two_files (char *path1, char *path2)
{
	Model *m;
	m = new Model();\
	puts (path1);
	puts (path2);
	IndexedFaceSet *ifs1 = stl_parser (path1, m);
	if (!ifs1)
		return NULL;

	IndexedFaceSet *ifs2 = stl_parser (path2, m);
	if (!ifs2) {
		delete ifs1;
		return NULL;
	}

	Node *node;
	node = new Node ();
	m->nodes = node;

	Transform *transform1 = new Transform ();
	Shape *shape1 = new Shape ();
	Appearance *appearance1 = new Appearance ();
	Material *material1 = new Material ();
	appearance1->add_child (material1);
	shape1->appearance = appearance1;
	appearance1->parent = shape1;
	transform1->add_child (shape1);
	node->add_child (transform1);
	shape1->add_geometry (ifs1);
	transform1->translate_y = -0.5f * (ifs1->maxy - ifs1->miny);
	transform1->operations [0] = Transform::TRANSLATE;
	transform1->op_index = 1;
	ifs1->parent = shape1;

	Transform *transform2 = new Transform ();
	Shape *shape2 = new Shape ();
	Appearance *appearance2 = new Appearance ();
	Material *material2 = new Material ();
	appearance2->add_child (material2);
	shape2->appearance = appearance2;
	appearance2->parent = shape2;
	transform2->add_child (shape2);
	node->add_child (transform2);
	shape2->add_geometry (ifs2);
	transform2->translate_y = 0.5f * (ifs2->maxy - ifs2->miny);
	transform2->operations [0] = Transform::TRANSLATE;
	transform2->op_index = 1;
	ifs2->parent = shape2;

	return m;
}



typedef struct {
	float nx,ny,nz;
	float x1,y1,z1;
	float x2,y2,z2;
	float x3,y3,z3;
} STLTRI;

void stl_export (IndexedFaceSet *ifs, char *path)
{

	float scale = 1000.f;

	FILE *fp = NULL;

	fp = fopen(path, "wb");

	if(!fp)
	{
		return;
	}

	if(!ifs)
		return;

	//write header
	char* header[80];
	memset(&header, 0xFF, sizeof(char) * 80);
	fwrite(header, sizeof(char), 80, fp);

	//tri count
	fwrite(&ifs->n_triangles, sizeof(unsigned int), 1, fp);
	
	//write triangles
	for(int i=0;i<ifs->n_triangles;i++) 
	{
		STLTRI t;
		memset(&t,0,sizeof(STLTRI));

		//each tri
		Triangle* tri = ifs->triangles[i];
		if(tri->normal_vector)
			t.nx = tri->normal_vector->x; t.ny = tri->normal_vector->y; t.nz = tri->normal_vector->z;

		t.x1 = tri->p1->x * scale; t.y1 = tri->p1->y * scale; t.z1 = tri->p1->z * scale;
		t.x2 = tri->p2->x * scale; t.y2 = tri->p2->y * scale; t.z2 = tri->p2->z * scale;
		t.x3 = tri->p3->x * scale; t.y3 = tri->p3->y * scale; t.z3 = tri->p3->z * scale;

		fwrite(&t, sizeof(STLTRI), 1, fp);

		//atrributes
		char attr[2];
		attr[0] = 0;
		attr[1] = 0;
		fwrite(attr, sizeof(char), 2, fp);
	}
	
	fclose(fp);

}

