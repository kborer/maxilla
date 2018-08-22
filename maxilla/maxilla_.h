/*=============================================================================
  Maxilla, an OpenGL-based 3D program for viewing dentistry-related VRML & STL.
  Copyright (C) 2008-2011 by Zack T Smith and Ortho Cast Inc.

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

/*
 * NOTICE NOTICE NOTICE
 *
 * Please do not use multiple inheritance or templates in this program.
 * They are the leading causes of C++ code illegibility.
 *
 * Keep this program clean! Don't pollute it.
 */

#ifndef _MAXILLA_H
#define _MAXILLA_H

#include <stdio.h>
#include <math.h>

#include "defs.h"
#include "quat.h"

#ifdef WIN32
#include "../../zlib/zlib.h"
#else 
#include <zlib.h>
#endif

#include "Point.h"

extern int serialization_indentation_level;
extern void indent (gzFile );

typedef unsigned long RGB;
extern RGB floats_to_rgb (float, float, float);
extern void express_rgb (RGB);

extern bool using_custom_colors;
extern GLfloat user_fg[4];
extern GLfloat user_bg[4];
extern GLfloat user_shininess;
extern GLfloat user_specularity;
extern GLfloat user_emissivity;

extern bool redrawing_for_selection;

extern float field_of_view;
extern float viewpoint_x;
extern float viewpoint_y;
extern float viewpoint_z;
extern float orientation_x;
extern float orientation_y;
extern float orientation_z;

extern void rotate_bounds_about_one_axis (double angle, char which,
	double& xmin, double& xmax, double& ymin, double& ymax, double& zmin, double &zmax);

class red_dot_info {
public:
	double x, y, z;
	red_dot_info *next;
	int name;

	red_dot_info () :
		x(0.), y(0.), z(0.), next(NULL)
	{
	}
	red_dot_info (double x_, double y_, double z_, int name_) :
		x(x_),
		y(y_),
		z(z_),
		name(name_),
		next(NULL)
	{
	}
};

extern red_dot_info *red_dot_stack;

//-------------------------------------------
// Camera characteristics.
//
class CameraCharacteristics {
public:
	int window;
	int configuration;	// e.g. SHOW_MAXILLA
	bool need_recenter;
	double field_of_view;
	double rotation_pitch;
	double rotation_yaw;
	Quat combined_rotation;
	Quat starting_rotation;
	double scale_factor;
	double viewpoint_x;
	double viewpoint_y;
	double viewpoint_z;
	double orientation_x;
	double orientation_y;
	double orientation_z;
	double viewpoints[3];

	CameraCharacteristics ();
};
extern CameraCharacteristics cc_main;

/*===========================================================================
 * Name:	ASSERT_NONZERO
 * Purpose:	Bails if parameter is zero.
 * Note:	To be used in important functions.
 */
#ifdef WIN32
#define ASSERT_NONZERO(PTR,NAME) \
	if(!(PTR)) {\
		fatal_null_pointer(__FUNCTION__, NAME); \
	}
#else
#define ASSERT_NONZERO(PTR,NAME) \
	if(!(PTR)) {\
		fatal_null_pointer(__FUNCTION__, NAME); \
	}
#endif

/*===========================================================================
 * Name:	ENTRY, ENTRY2, REPORT2
 * Purpose:	Debugging code that records the order of calls to files.
 */
#if 0
	#define ENTRY {FILE *ff=fopen("c:/maxilla_express.txt","a");fprintf(ff,"Entered %s, type=%s, name=%s\n",__FUNCTION__,type,name);fclose(ff);}
	#define ENTRY2 {int ii=level; FILE *ff=fopen("c:/maxilla_bounds.txt","a");while(ii>0){ii--;fprintf(ff,"  ");}fprintf(ff,"Entered %s, type=%s, name=%s\n",__FUNCTION__,type,name);fclose(ff);}
	#define REPORT2 {int ii=level; FILE *ff=fopen("c:/maxilla_bounds.txt","a");while(ii>0){ii--;fprintf(ff,"  ");}fprintf(ff,"Reporting type=%s name=%s: %g,%g  %g,%g  %g,%g\n",type,name,minx,maxx,miny,maxy,minz,maxz);fclose(ff);}
#else
	#define ENTRY
	#define ENTRY2
	#define REPORT2
#endif

/*===========================================================================
 * Name:	WhiteLight
 * Purpose:	Holds light info w/intent that user can change it on the fly.
 */
class WhiteLight {
public:
	// Values currently in use.
	float pos[4];
	float brightness, specularity;
	float ambient;
	bool positional;

	// Default ortho values, copied in when orthographic mode enabled.
	float pos_ortho[4];
	float brightness_ortho;
	float specularity_ortho;
	float ambient_ortho;

	// Default non-ortho values, copied in when orthographic mode disabled.
	float pos_non_ortho[4];
	float brightness_non_ortho;
	float specularity_non_ortho;
	float ambient_non_ortho;

	// Default values for cross-section mode.
	float pos_cross_section[4];
	float brightness_cross_section;
	float specularity_cross_section;
	float ambient_cross_section;

	WhiteLight (bool positional_) {
		int i;
		for (i=0; i<3; i++) {
			pos[i] = 0.f;
			pos_non_ortho[i] = 0.f;
			pos_ortho[i] = 0.f;
		}
		pos[3] = pos_ortho[3] = pos_non_ortho[3] = positional_ ? 1.f : 0.f;
		positional = positional_;
		brightness = brightness_ortho = brightness_non_ortho = 0.1f;
		specularity = specularity_ortho = specularity_non_ortho = 0.1f;
		ambient = ambient_ortho = ambient_non_ortho = 0.1f;

		total_allocated += sizeof(WhiteLight);
	}

	/*===================================================================
	 * Name:	dump
	 * Purpose:	Prints information about light's characteristics.
	 */
	void dump (int num) {
		printf ("%s light %d: pos %g,%g,%g, brightness %g, specularity %g, ambient %g\n",
			positional ? "Positional" : "Directional", 
			num, pos[0], pos[1], pos[2], brightness, specularity, ambient);
	}

	/*===================================================================
	 * Name:	reset
	 * Purpose:	Resets light to original ortho or non-ortho values.
	 */
	void reset (char mode) {
		switch (mode) {
		case 'o':
			pos[0] = pos_ortho[0];
			pos[1] = pos_ortho[1];
			pos[2] = pos_ortho[2];
			pos[3] = pos_ortho[3];
			brightness = brightness_ortho;
			specularity = specularity_ortho;
			ambient = ambient_ortho;
			break;
		case 'n':
			pos[0] = pos_non_ortho[0];
			pos[1] = pos_non_ortho[1];
			pos[2] = pos_non_ortho[2];
			pos[3] = pos_non_ortho[3];
			brightness = brightness_non_ortho;
			specularity = specularity_non_ortho;
			ambient = ambient_non_ortho;
		case 'c':
			pos[0] = pos_cross_section[0];
			pos[1] = pos_cross_section[1];
			pos[2] = pos_cross_section[2];
			pos[3] = pos_cross_section[3];
			brightness = brightness_cross_section;
			specularity = specularity_cross_section;
			ambient = ambient_cross_section;
		}
	}

	/*===================================================================
	 * Name:	express
	 * Purpose:	Expresses the light's characteristics as OpenGL calls.
	 */
	void express (int num) {
		static int lights[4] = { 
			GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3 
		};
		int which = lights[num];
		float tmp[4];

		tmp[0] = tmp[1] = tmp[2] = ambient;
		tmp[3] = 1.f;
		glLightfv (which, GL_AMBIENT, tmp);

		tmp[0] = tmp[1] = tmp[2] = specularity;
		tmp[3] = 1.f;
		glLightfv (which, GL_SPECULAR, tmp);

		tmp[0] = tmp[1] = tmp[2] = brightness;
		tmp[3] = 1.f;
		glLightfv (which, GL_DIFFUSE, tmp);

		glLightfv (which, GL_POSITION, pos);
	}
};

class Model;

/*===========================================================================
 * Name:	Triangle
 * Purpose:	Encapsulates a triangle i.e. a face.
 */
class Triangle 
{
	public:
		Point *p1, *p2, *p3;
		Point *normal_vector;
		Model *model;
		int indices [3];
		float area;

		Triangle () {
			p1 = p2 = p3 = normal_vector = NULL;
			model = NULL;
			area = 0.f;
		}

		/*===================================================================
		 * Name:	Triangle
		 * Purpose:	Creates the object and computes normal vector.
		 */
		Triangle (Point *p1_, Point *p2_, Point *p3_);

		/*===================================================================
		 * Name:	serialize
		 * Purpose:	Express the triangle as ASCII.
		 */
		void serialize (gzFile f) 
		{
			if (!f) 
				return;
			if (!p1 || !p2 || p3) {
				warning ("Cannot serialize triangle, it is missing points.");
				return;
			}
			gzprintf (f, "%d %d %d\n", p1->id, p2->id, p3->id);
		}

		/*===================================================================
		 * Name:	express
		 * Purpose:	Expresses the triangle as OpenGL calls.
		 */
		void express ();
		void express_as_lines ();

		/*===================================================================
		 * Name:	get_center
		 * Purpose:	Finds the center of a triangle.
		 */
		void get_center (double &x, double &y, double &z) {
			x = (p1->x + p2->x + p3->x) / 3.f;
			y = (p1->y + p2->y + p3->y) / 3.f;
			z = (p1->z + p2->z + p3->z) / 3.f;
		}
};


/*===========================================================================
 * Name:	PolyLine
 * Purpose:	Encapsulates a polyline as a linked list.
 */
class PolyLine 
{
public:
	Point *point;
	PolyLine *next;
	float color[4];

	PolyLine() 
	{
		point = NULL;
		next = NULL;
	}

	PolyLine (Point *p_) 
	{
		point = p_;
		next = NULL;
	}

	~PolyLine() 
	{
		// point is allocated by someone else.
		delete next;
	}

	/*===================================================================
	 * Name:	express
	 * Purpose:	Expresses the polyline recursively as OpenGL calls.
	 */
	void express (bool use_color) 
	{
		if (use_color)
			return;
		if (point)
			Point_express (point);
		if (next)
			next->express (use_color);
	}
};


/*===========================================================================
 * Name:	Node
 * Purpose:	Encapsulates a parsed version of information about 
 *		the objects found in the VRML file.
 */
class Node {
public:
	const char *type;
	Node *next;
	Node *children;
	Node *last_child;
	Node *parent;
	char *name;

	bool dont_save_this_node;

	Node () {
		type = "Node";
		name = NULL;
		next = NULL;
		children = NULL;
		last_child = NULL;
		parent = NULL;
		dont_save_this_node = false;

		total_allocated += sizeof(Node);
	}

	~Node() {
		total_allocated -= sizeof(Node);

		if (children)
			delete children;
		children = NULL;
		last_child = NULL;
		if (next)
			delete next;
		next = NULL;
		// name is not a copy.
		// type is a const.
	}

	/*===================================================================
	 * Name:	find_by_name
	 * Purpose:	Finds a node by its name.
	 * Returns:	Node pointer or NULL if not found.
	 */
	Node *find_by_name (char *s) {
		if (name && !strcmp (s, name))
			return this;
		Node *result = NULL;
		if (next) 
			result = next->find_by_name (s);
		if (!result && children) 
			result = children->find_by_name (s);
		return result;
	}

	/*===================================================================
	 * Name:	find_by_type
	 * Purpose:	Finds the first node of the specified type.
	 * Returns:	Node pointer or NULL if not found.
	 */
	Node *find_by_type (char *s) {
		if (type && !strcmp (s, type))
			return this;
		Node *result = NULL;
		if (next) 
			result = next->find_by_type (s);
		if (!result && children) 
			result = children->find_by_type (s);
		return result;
	}

	/*===================================================================
	 * Name:	add_child
	 * Purpose:	Appends a child to the Node's children linked list.
	 * Returns:	The total # of children.
	 */
	int add_child (Node *n) 
	{
		ASSERT_NONZERO(n,"node")
		//----------

		if (!children)
			children = n;
		else
			last_child->next = n;
		last_child = n;

		int count = 0;
		Node *node = children;
		while (node) {
			count++;
			node = node->next;
		}
		return count;
	}

	/*===================================================================
	 * Name:	express
	 * Purpose:	Expresses the object in terms of OpenGL calls.
	 */
	virtual void express (bool express_siblings) {
		ENTRY
		if (children)
			children->express (true);
		if (express_siblings && next)
			next->express (true);
	}

	void serialize (gzFile f);

	/*===================================================================
	 * Name:	dump
	 * Purpose:	Diagnostic dump for testing.
	 */
	virtual void dump (FILE *f) 
	{
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf (f, "%s %s\n", type, name? name : "");
	}

	/*===================================================================
	 * Name:	smooth
	 * Purpose:	Smooths nodes.
	 */
	virtual void smooth ();

	/*===================================================================
	 * Name:	report_bounds
	 * Purpose:	Reports to caller maximum and minimum coordinates.
	 */
	virtual void report_bounds (int level, bool report_siblings,
				double &minx, double &maxx,
				double &miny, double &maxy,
				double &minz, double &maxz) 
	{
		ENTRY2
		double my_xmin, my_xmax, my_ymin, my_ymax, my_zmin, my_zmax;
		double xmin2, xmax2, ymin2, ymax2, zmin2, zmax2;

		my_xmin = my_ymin = my_zmin = xmin2 = ymin2 = zmin2 = 1E6f;
		my_xmax = my_ymax = my_zmax = xmax2 = ymax2 = zmax2 = -1E6f;

		if (children) {
			children->report_bounds (level+1, true,
						my_xmin, my_xmax, my_ymin, my_ymax, my_zmin, my_zmax);
		}

		if (report_siblings && next) {
			next->report_bounds (level, true, 
						xmin2, xmax2, ymin2, ymax2, zmin2, zmax2);
			if (xmin2 < my_xmin)
				my_xmin = xmin2;
			if (ymin2 < my_ymin)
				my_ymin = ymin2;
			if (zmin2 < my_zmin)
				my_zmin = zmin2;
			if (xmax2 > my_xmax)
				my_xmax = xmax2;
			if (ymax2 > my_ymax)
				my_ymax = ymax2;
			if (zmax2 > my_zmax)
				my_zmax = zmax2;
		}

		minx = my_xmin;
		maxx = my_xmax;
		miny = my_ymin;
		maxy = my_ymax;
		minz = my_zmin;
		maxz = my_zmax;
		REPORT2
	}
};


/*===========================================================================
 * Name:	NameMap
 * Purpose:	Struct for implementing a simple linked list of 
 *			name-value associations.
 * Note:	Do not replace with a templated class!
 */
struct NameMap {
	char *name;
	Node *node;
	NameMap *next;
};


/*===========================================================================
 * Name:	InputWord
 * Purpose:	Encapsulates a string that was read from a VRML file.
 */
class InputWord {
public:
	char *str;
	float value;
	InputWord *next;
	InputWord *children;

	InputWord (const char*);
	InputWord (float num) {
		str = "#";
		value = num;
		next = children = NULL;

		total_allocated += sizeof(InputWord);
	}
	~InputWord ();
};

/*===========================================================================
 * Name:	InputFile
 * Purpose:	Encapsulates information about an input file.
 */
class InputFile {
public:
	bool valid;
	char *path;
	char *name;
	char *extension;
	short ungot_char;
	gzFile gzfile;	// Used whether compressed or not.
	InputWord *tree;
	unsigned char buffer[INPUTFILE_BUFFERSIZE];
	int buffer_ix;
	int buffer_limit;
	char *first_line;
	bool is_dst_file;

	InputFile(char*);

	~InputFile() {
		total_allocated -= path? strlen(path) + 1 : 0;
		total_allocated -= first_line? strlen(first_line) + 1 : 0;

		if (path)
			free(path);
		if (first_line)
			free(first_line);
	}

	/*===================================================================
	 * Name:	unget
	 * Purpose:	Reverses the last read. Limit 1 character.
	 */
	void unget (char ch) {
		if (ch != EOF) {
			if (ungot_char != EOF)
				warning ("InputFile::unget called twice.");
			else
				ungot_char = ch;
		}
	}

	/*===================================================================
	 * Name:	getchar
	 * Purpose:	Fetches one character from the buffered file.
	 */
	int getchar() {
		int ch = EOF;
		if (ungot_char != EOF) {
			short tmp = ungot_char;
			ungot_char = EOF;
			return tmp;
		}
		if (buffer_limit >=0 && buffer_ix >= 0)	{
			if (buffer_ix >= buffer_limit) {
				if (!replenish_buffer())
					return EOF;
			}
			ch = buffer[buffer_ix++];
		}
		return ch;
	}

	/*===================================================================
	 * Name:	open
	 * Purpose:	Opens the file, assuming the path is already set.
	 */
	bool open() {
		if (!path)
			return false;
		gzfile = gzopen (path, "rb");
		if (gzfile)
			replenish_buffer();
		return gzfile != NULL;
	}

	/*===================================================================
	 * Name:	close
	 * Purpose:	Closes the file, if open.
	 */
	void close() {
		if (gzfile)
			gzclose(gzfile);
		gzfile = NULL;
		buffer_ix = buffer_limit = -1;
	}

private:
	/*===================================================================
	 * Name:	replenish_buffer
	 * Purpose:	Loads more data from the file into the buffer.
	 */
	bool replenish_buffer () 
	{
		buffer_ix = 0;
		buffer_limit = -1;

		if (gzfile)
			buffer_limit = gzread (gzfile, buffer, INPUTFILE_BUFFERSIZE);		

		// If this is the first-ever read, let's
		// store the 1st line.
		//
		if (!first_line) {
			int i=0;
			unsigned long chunk_size=0;
			// Find the 1st newline. (Windows does not have strnchr().)
			while (buffer[i] != '\n' && i<INPUTFILE_BUFFERSIZE)
				i++;
			chunk_size = i + 1;
			first_line = (char*) malloc(chunk_size);
			memcpy (first_line, buffer, chunk_size);
			first_line[chunk_size-1] = 0;

			total_allocated += chunk_size;
		}

		return buffer_limit > 0;
	}

};

/*===========================================================================
 * Name:	Transform
 * Purpose:	Represents a VRML Transform node.
 */
class Transform : public Node {
public:
	double translate_x;
	double translate_y;
	double translate_z;
	double second_translate_x;
	double second_translate_y;
	double second_translate_z;
	double scale_x;
	double scale_y;
	double scale_z;
	double rotate_x;
	double rotate_y;
	double rotate_z;
	double rotate_angle; // in radians
	double second_rotate_x;
	double second_rotate_y;
	double second_rotate_z;
	double second_rotate_angle; // in radians
	double center_x;
	double center_y;
	double center_z;

	// The following are used to keep track of the order of operations.
	// RULE: In VRML's Transform construct, the order matters.
	//
#define TRANSFORM_MAX_OPS 4
	enum { TRANSLATE=1, SCALE=2, ROTATE=3, ROTATE2=4, TRANSLATE2=5 };
	char operations [TRANSFORM_MAX_OPS];
	unsigned short op_index;

	Transform() {
		type = "Transform";
		translate_x = 0.0f;
		translate_y = 0.0f;
		translate_z = 0.0f;
		second_translate_x = 0.0f;
		second_translate_y = 0.0f;
		second_translate_z = 0.0f;
		scale_x = 1.0f;
		scale_y = 1.0f;
		scale_z = 1.0f;
		rotate_x = 0.0f;
		rotate_y = 0.0f;
		rotate_z = 0.0f;
		second_rotate_x = 0.0f;
		second_rotate_y = 0.0f;
		second_rotate_z = 0.0f;
		center_x = center_y = center_z = 0.f;
		rotate_angle = 0.0f;
		children = NULL;
		next = NULL;
		op_index = 0; // No VRML operations yet recorded.

		total_allocated += sizeof(Transform);
	}

	void serialize (gzFile );

	/*===================================================================
	 * Name:	express
	 * Purpose:	Creates a local OpenGL translate & scale.
	 */
	void express (bool express_siblings) {
		int i;

		ENTRY
		glPushMatrix ();

		// Express translate/rotate/scale.
		// Note that OpenGL processes any translate/rotate/scale in reverse order,
		// whereas VRML expresses them in forward order.
		//
		i = op_index - 1;
		while (i >= 0) {
			switch (operations[i]) 
			{
			case ROTATE:
				if (rotate_angle != 0.0f) 
					glRotatef ((rotate_angle/(float)M_PI)*180.f, rotate_x, rotate_y, rotate_z);
				break;

			case ROTATE2:
				if (second_rotate_angle != 0.0f) 
					glRotatef ((second_rotate_angle/(float)M_PI)*180.f, 
						second_rotate_x, second_rotate_y, second_rotate_z);
				break;

			case SCALE:
				glScalef (scale_x, scale_y, scale_z);
				break;
	
			case TRANSLATE:
				glTranslatef (translate_x, translate_y, translate_z);
				break;

			case TRANSLATE2:
				glTranslatef (second_translate_x, second_translate_y, second_translate_z);
				break;
			}
			i--;
		}

#if 0
		if (center_x != 0.f || center_y != 0.f || center_z != 0.f)
			glTranslatef (-center_x, -center_y, -center_z);
#endif
		if (children)
			children->express (true);

		glPopMatrix ();

		if (express_siblings && next)
			next->express (true);
	}

	/*===================================================================
	 * Name:	dump
	 * Purpose:	Diagnostic dump.
	 */
	void dump (FILE *f) {
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf (f, "%s %s: translate %g %g %g rotate %g %g %g (%g deg) scale %g %g %g\n", 
			type, name? name : "",
			translate_x, translate_y, translate_z,
			rotate_x, rotate_y, rotate_z, rotate_angle,
			scale_x, scale_y, scale_z);
	}

	/*===================================================================
	 * Name:	report_bounds
	 * Purpose:	Reports to caller maximum and minimum coordinates.
	 */
	void report_bounds (int level, bool report_siblings,
				double &minx, double &maxx,
				double &miny, double &maxy,
				double &minz, double &maxz) 
	{
		ENTRY2

		double my_xmin, my_xmax, my_ymin, my_ymax, my_zmin, my_zmax;
		my_xmin = my_ymin = my_zmin = 1E6f;
		my_xmax = my_ymax = my_zmax = -1E6f;

		if (children) {
			children->report_bounds (level+1, true,
			 my_xmin, my_xmax, my_ymin, my_ymax, my_zmin, my_zmax);
		}

		// Perform translate/rotate/scale on bounding box.
		// Note that OpenGL processes any translate/rotate/scale in reverse order,
		// whereas VRML expresses them in forward order.
		//
		if (my_xmin < 1E6f && my_ymin < 1E6f && my_zmin < 1E6f) {
			int i = 0;
			while (i < op_index) {
				switch (operations[i]) {
				case ROTATE:	
					if (rotate_angle != 0.f) {
						double tmp = rotate_angle - (float) M_PI;
						if (tmp < 0)
							tmp = -tmp;
	
						// At present we deal only with the case of 180 degree rotation.
#if 0
						if (rotate_z == 1.f && (tmp < 0.0001)) {
							double tmp = -my_ymin;
							double tmp2 = -my_ymax;
							my_ymin = tmp2;
							my_ymax = tmp;
						}
						else 
#endif
							if (rotate_x == 1.f && rotate_z == 0.f && rotate_y == 0.f) {
							rotate_bounds_about_one_axis (rotate_angle, 'x',
								my_xmin, my_xmax, my_ymin, my_ymax, my_zmin, my_zmax);
						}
						else if (rotate_y == 1.f && rotate_x == 0.f && rotate_z == 0.f) {
							rotate_bounds_about_one_axis (rotate_angle, 'y',
								my_xmin, my_xmax, my_ymin, my_ymax, my_zmin, my_zmax);
						}
						else if (rotate_z == 1.f && rotate_x == 0.f && rotate_y == 0.f) {
							rotate_bounds_about_one_axis (rotate_angle, 'z',
								my_xmin, my_xmax, my_ymin, my_ymax, my_zmin, my_zmax);
						}
						else {
							// Rotate about an arbitrary axis not yet supported. XX
							printf ("Encountered arbitrary rotation... ignoring.\n");
						}
					}
					break;

				case SCALE:
					my_xmin *= scale_x;
					my_xmax *= scale_x;
					my_ymin *= scale_y;
					my_ymax *= scale_y;
					my_zmin *= scale_z;
					my_zmax *= scale_z;	
					break;
	
				case TRANSLATE:
					my_xmin += translate_x;
					my_xmax += translate_x;
					my_ymin += translate_y;
					my_ymax += translate_y;
					my_zmin += translate_z;
					my_zmax += translate_z;
					break;
				}
				i++;
			}
		}

		if (next && report_siblings) {
			double minx2, maxx2, miny2, maxy2, minz2, maxz2;
			minx2 = miny2 = minz2 = 1E6f;
			maxx2 = maxy2 = maxz2 = -1E6f;

			next->report_bounds (level, true, minx2, maxx2, miny2, maxy2, minz2, maxz2);
			if (minx2 < my_xmin)
				my_xmin = minx2;
			if (miny2 < my_ymin)
				my_ymin = miny2;
			if (minz2 < my_zmin)
				my_zmin = minz2;

			if (maxx2 > my_xmax)
				my_xmax = maxx2;
			if (maxy2 > my_ymax)
				my_ymax = maxy2;
			if (maxz2 > my_zmax)
				my_zmax = maxz2;
		}

		minx = my_xmin;
		miny = my_ymin;
		minz = my_zmin;

		maxx = my_xmax;
		maxy = my_ymax;
		maxz = my_zmax;

		REPORT2
	}
};


/*===========================================================================
 * Name:	Switch
 * Purpose:	Represents a VRML Switch node.
 */
class Switch : public Node {
public:
	int which;
#ifdef ORTHOCAST
	bool doing_open_view;
#endif
	int total_choices;
	Node *which_node;

	Switch() 
	{
		type = "Switch";

		// No node yet chosen.
		which = -1;
		which_node = NULL;

		total_choices = 0;
		children = NULL;
		next = NULL;

		total_allocated += sizeof(Switch);
	}

	/*===================================================================
	 * Name:	express
	 * Purpose:	Expresses Switch contents unto OpenGL.
	 */
	void express (bool express_siblings) 
	{
		ENTRY

		// Update just in case it was cleared.
		if (!which_node)
			update_which_node();

		// If we have a which_node, use it and
		// tell the node not to express siblings.
		// 
		if (which_node) {
			which_node->express (false);
			printf ("expressing which node (%s) in Switch\n", which_node->type);
		}

		// If not, we never express the next node in a Switch.
	}

	/*===================================================================
	 * Name:	report_bounds
	 * Purpose:	Reports to caller maximum and minimum coordinates.
	 */
	void report_bounds (int level, bool report_siblings,
				double &minx, double &maxx,
				double &miny, double &maxy,
				double &minz, double &maxz) 
	{
		ENTRY2

		double my_xmin, my_xmax, my_ymin, my_ymax, my_zmin, my_zmax;
		my_xmin = my_ymin = my_zmin = 1E6f;
		my_xmax = my_ymax = my_zmax = -1E6f;

		if (!which_node)
			update_which_node();

		if (which_node)
			which_node->report_bounds (level+1, false,
					my_xmin, my_xmax, my_ymin, my_ymax, my_zmin, my_zmax);

		if (next && report_siblings) {
			double minx2, maxx2, miny2, maxy2, minz2, maxz2;
			minx2 = miny2 = minz2 = 1E6f;
			maxx2 = maxy2 = maxz2 = -1E6f;

			next->report_bounds (level, true, minx2, maxx2, miny2, maxy2, minz2, maxz2);
			
			if (minx2 < my_xmin)
				my_xmin = minx2;
			if (miny2 < my_ymin)
				my_ymin = miny2;
			if (minz2 < my_zmin)
				my_zmin = minz2;

			if (maxx2 > my_xmax)
				my_xmax = maxx2;
			if (maxy2 > my_ymax)
				my_ymax = maxy2;
			if (maxz2 > my_zmax)
				my_zmax = maxz2;
		}

		minx = my_xmin;
		miny = my_ymin;
		minz = my_zmin;

		maxx = my_xmax;
		maxy = my_ymax;
		maxz = my_zmax;

		REPORT2
	}

	/*===================================================================
	 * Name:	dump
	 * Purpose:	Diagnostic dump.
	 */
	void dump (FILE *f) 
	{
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf (f, "%s %s (which = %d)\n",
			type, name? name : "",
			which);
	}

	/*===================================================================
	 * Name:	update_which_node
	 * Purpose:	Diagnostic dump.
	 */
	Node *update_which_node() {
		//----------------------------------------
		// Find the nth node, as specified w/ VRML
		// 'whichChoice' value.
		//
		Node *n = children;
		int i = which;
		while (i > 0 && n) {
			i--;
			n = n->next;
		}
		which_node = n;
// printf (" which node %08lx\n", (long)n);
		return n;
	}
};

class Group;

/*===========================================================================
 * Name:	Model
 * Purpose:	Encapsulates a 3D model, specifically a VRML model.
 */
class Model {
public:
	char *title; // not strdup'd

	bool smoothed;

	bool background_provided; 
	GLfloat bg[3]; // read from VRML file.

	double minx, maxx, miny, maxy, minz, maxz;

	InputFile *inputfile;	// file
	InputWord *word_tree;	// data from file
	Node *nodes;	// data parsed from data from file

#ifdef ORTHOCAST
	// Some dentistry-specific data.
	Node *main_switch;
	char *patient_firstname;
	char *patient_lastname;
	char *patient_birthdate;
	Group *top_and_bottom_node;	
	char *case_date;
	char *case_number;
	char *control_number;

	bool isTopAndBottomAligned;		

#endif

	// Overall translation to center the model at (0,0,0).
	double translate_x;
	double translate_y;
	double translate_z;

	// Map of names to Nodes for VRML "USE".
	NameMap *names;
	NameMap *last_name;

	Model () :
		inputfile(NULL),
		background_provided(false),
		title(NULL),
		names(NULL), last_name(NULL),
		word_tree(NULL),
		nodes(NULL),
#ifdef ORTHOCAST
		main_switch(NULL),
		patient_firstname(NULL),
		patient_lastname(NULL),
		patient_birthdate(NULL),
		top_and_bottom_node(NULL),	
		case_date(NULL),
		case_number(NULL),
		control_number(NULL),
		isTopAndBottomAligned(false),		
#endif
		smoothed(false),
		translate_x(0.f),
		translate_y(0.f),
		translate_z(0.f),
		maxx(-1E6f), maxy(-1E6f), maxz(-1E6f), 
		minx(1E6f), miny(1E6f), minz(1E6f)
	{
		bg[0] = bg[1] = bg[2] = 0.f;

		total_allocated += sizeof(Model);
	}

	/*===================================================================
	 * Name:	register_switch
	 * Purpose:	Tells the Model that a switch node exists.
	 */
	void register_switch (Switch *sw)
	{
		ASSERT_NONZERO(sw,"switch");

		if (!main_switch)
			main_switch = sw;
	}

	/*===================================================================
	 * Name:	translate_all
	 * Purpose:	Permits a one-time recentering of a parsed model.
	 */
	bool translate_all (double x, double y, double z) {
		translate_x = x;
		translate_y = y;
		translate_z = z;
		return true;
	}

	/*===================================================================
	 * Name:	add_name_mapping
	 * Purpose:	Maps a name to a node, for use with "USE" construct.
	 */
	void add_name_mapping (char *name, Node *n) 
	{
		ASSERT_NONZERO(name,"name")
		ASSERT_NONZERO(n,"node")
		//----------

		if (!name || !n)
			fatal("Null param in add_name_mapping.");

		NameMap *m = new NameMap();
		total_allocated += sizeof(NameMap);

		m->name = name;
		m->node = n;
		if (!names)
			names = m;
		else
			last_name->next = m;
		last_name = m;
	}

	/*===================================================================
	 * Name:	lookup_node_by_name
	 * Purpose:	Finds a node by its name.
	 * Note:	It's slow but there usually aren't many names.
	 */
	Node* lookup_node_by_name (char *name) 
	{
		ASSERT_NONZERO(name,"name")
		//----------

		NameMap *m = names;
		while(m) {
			if (!strcmp(name,m->name))
				return m->node;
			m = m->next;
		}
		return NULL;
	}

	/*===================================================================
	 * Name:	smooth
	 * Purpose:	Performs smoothing.
	 */
	void smooth ();

	/*===================================================================
	 * Name:	express
	 * Purpose:	Expresses model's objects in terms of OpenGL calls.
	 */
	void express ();

	/*===================================================================
	 * Name:	report_bounds
	 * Purpose:	Determines the bounding box for the model, given the
	 *		current switch choices.
	 */
	void report_bounds ();

	/*===================================================================
	 * Name:	which_switch_has_triangle
	 * Purpose:	Tells the caller which Switch contains the specified
	 *		Triangle object.
	 */
	int which_switch_has_triangle (Triangle *t);

#if 0
	/*===================================================================
	 * Name:	ensure_tiny_triangles
	 * Purpose:	Breaks IndexedFaceSets' triangles into small ones.
	 */
	bool ensure_tiny_triangles (double maximum);
	bool ensure_tiny_triangles_core (double maximum, Node *);
#endif

	/*===================================================================
	 * Name:	~Model
	 * Purpose:	Carefully cleans up when Model to avoid memory leaks.
	 */
	~Model() {
	//	delete word_tree;  -- XX
		delete nodes;

		total_allocated -=  sizeof(Model);

		NameMap *m = names;
		while (m) {
			NameMap *m2 = m->next;
			delete m;
			m = m2;

			total_allocated -= sizeof(NameMap);
		}
	}

	/*===================================================================
	 * Name:	serialize
	 * Purpose:	Express the entire model as ASCII.
	 */
	void serialize (gzFile f) 
	{
		if (!f) 
			return;
		
		//----------------------------------------
		// Write out the beginning VRML.
		//
		serialization_indentation_level = 0;

		nodes->serialize (f);

		//----------------------------------------
		// Write out the lower.
		//

		//----------------------------------------
		// Write out the upper.
		//

		//----------------------------------------
		// Write out the upper-lower alignment.
		//

		//----------------------------------------
		// Write out the ending VRML.
		//

		gzprintf (f, "\n\n\n");
	}


	void autocenter ();
};


/*===========================================================================
 * Name:	Group
 * Purpose:	Represents a VRML Group node.
 */
class Group : public Node {
public:
	Group() {
		type = "Group";
		children = NULL;
		next = NULL;

		total_allocated += sizeof(Group);
	}

	/*===================================================================
	 * Name:	dump
	 * Purpose:	Diagnostic dump.
	 */
	void dump (FILE *f) {
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf (f, "%s %s\n",
			type, name? name : "");
	}
};


/*===========================================================================
 * Name:	Separator
 * Purpose:	Represents a VRML Separator node.
 */
class Separator : public Node {
public:
	Separator() {
		type = "Separator";
		children = NULL;
		next = NULL;

		total_allocated += sizeof(Separator);
	}

	void express (bool express_siblings) {
		// Need to express various Separator characteristics.
		ENTRY
		if (children)
			children->express (true);
		if (express_siblings && next)
			next->express (true);
	}

	/*===================================================================
	 * Name:	dump
	 * Purpose:	Diagnostic dump.
	 */
	void dump (FILE *f) {
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf (f, "%s %s\n",
			type, name? name : "");
	}

};


/*===========================================================================
 * Name:	Material
 * Purpose:	
 */
class Material : public Node {
public:	
	GLfloat diffuseColor[4];
	GLfloat ambientIntensity;
	GLfloat specularColor[4];
	GLfloat emissiveColor[4];
	GLfloat shininess;
	GLfloat transparency;

	Material ()
	{
		type = "Material";

		for (int i=0; i < 4; i++) {
			diffuseColor[i] = 0.7f;
			specularColor[i] = emissiveColor[i] = 0.f;
		}
		ambientIntensity = 1.f;
		shininess = 1.f;
		transparency = 0.f;
	}

	~Material ()
	{
	}

	void express_colors ()
	{
		int i;
		GLfloat materialColor[4];
		GLfloat materialSpecular[4];
		GLfloat materialEmission[4];
		if (using_custom_colors) {
			for (i=0; i<3; i++) {
				materialColor[i] = user_fg[i];
				materialSpecular[i] = user_specularity;
				materialEmission[i] = user_emissivity;
			}
		} else {
			for (i=0; i<3; i++) {
				materialColor[i] = diffuseColor[i];
				materialSpecular[i] = specularColor[i];
				materialEmission[i] = emissiveColor[i];
			}
		}
		materialSpecular[3] = 1.0f;
		materialEmission[3] = 1.0f;
		materialColor[3] = ambientIntensity;

		glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
				materialColor);
		glMaterialfv (GL_FRONT, GL_SPECULAR, materialSpecular);
		glMaterialfv (GL_FRONT, GL_EMISSION, materialEmission);

		if (!using_custom_colors)
			glMaterialf (GL_FRONT, GL_SHININESS, shininess);
		else
			glMaterialf (GL_FRONT, GL_SHININESS, 
				user_shininess);
	}
};

/*===========================================================================
 * Name:	Appearance
 * Purpose:	
 */
class Appearance : public Node {
public:
	Material *material;

	Appearance () :
		material(NULL)
	{	
		type = "Appearance";
	}

	~Appearance ()
	{
		if (material)
			delete material;
	}
};


/*===========================================================================
 * Name:	Shape
 * Purpose:	Represents a VRML Shape node.
 * Note:	The 'children' variable points to the Shape's geometry.
 */
class Shape : public Node {
public:
	Appearance *appearance;

	Shape() :
		appearance(NULL)
	{
		type = "Shape";
		children = NULL;
		next = NULL;

		total_allocated += sizeof(Shape);
	}

	/*===================================================================
	 * Name:	~Shape
	 * Purpose:	Carefully deallocates Shape object to avoid memory leaks.
	 */
	~Shape() {
		if (children)
			delete children;
		children = NULL;
		last_child = NULL;
		if (appearance)
			delete appearance;
		if (next)
			delete next;
		next = NULL;
		// name is not a copy.
		// type is a const.

		total_allocated -= sizeof(Shape);
	}

	/*===================================================================
	 * Name:	add_geometry
	 * Purpose:	Sets the one geometry that a Shape is allowed.
	 */
	void add_geometry (Node *n) 
	{
		ASSERT_NONZERO(n,"node")
		//----------

		if (children != NULL)
			warning ("Attempt to add geometry to Shape that already has one.");
		else
			children = n;
	}

	void express_colors ()
	{
		if (appearance && appearance->material)
			appearance->material->express_colors ();
		else {
			int i;
			GLfloat materialColor[4];
			GLfloat materialSpecular[4];
			GLfloat materialEmission[4];
			for (i=0; i<3; i++) {
				materialColor[i] = user_fg[i];
				materialSpecular[i] = user_specularity;
				materialEmission[i] = user_emissivity;
			}
			materialSpecular[3] = 1.0f;
			materialEmission[3] = 1.0f;
			materialColor[3] = 0.5f; // ambient intensity

			glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
					materialColor);
			glMaterialfv (GL_FRONT, GL_SPECULAR, materialSpecular);
			glMaterialfv (GL_FRONT, GL_EMISSION, materialEmission);
			glMaterialf (GL_FRONT, GL_SHININESS, user_shininess);
		}
	}

	/*===================================================================
	 * Name:	express
	 * Purpose:	Expresses the Shape as OpenGL if geometry set.
	 */
	void express (bool express_siblings) {
		ENTRY

		if (children) {
			express_colors();
			children->express (true);
		}
		if (express_siblings && next)
			next->express (true);
	}

	/*===================================================================
	 * Name:	report_bounds
	 * Purpose:	Reports to caller maximum and minimum coordinates.
	 */
	void report_bounds (int level, bool report_siblings,
				double &minx, double &maxx,
				double &miny, double &maxy,
				double &minz, double &maxz) 
	{
		ENTRY2

		double xmin, xmax, ymin, ymax, zmin, zmax;
		xmin = ymin = zmin = 1E6f;
		xmax = ymax = zmax = -1E6f;

		if (children) {
			children->report_bounds (level+1, false,
							xmin, xmax, 
							ymin, ymax, 
							zmin, zmax);
		}

		// Get the number for the Shape's geometry node.
		//
		if (report_siblings && next) {
			double xming, xmaxg, yming, ymaxg, zming, zmaxg;
			xming = yming = zming = 1E6f;
			xmaxg = ymaxg = zmaxg = -1E6f;

			next->report_bounds (level, true,
					xming, xmaxg, yming, ymaxg, zming, zmaxg);

			if (xming < xmin)
				xmin = xming;
			if (yming < ymin)
				ymin = yming;
			if (zming < zmin)
				zmin = zming;

			if (xmaxg > xmax)
				xmax = xmaxg;
			if (ymaxg > ymax)
				ymax = ymaxg;
			if (zmaxg > zmax)
				zmax = zmaxg;
		}
		
		minx = xmin;
		miny = ymin;
		minz = zmin;
		maxx = xmax;
		maxy = ymax;
		maxz = zmax;

		REPORT2
	}

	/*===================================================================
	 * Name:	dump
	 * Purpose:	Diagnostic dump, makes sure to print children.
	 */
	void dump (FILE *f) {
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf (f, "%s %s: ", type, name? name : "");
		if (!children)
			fprintf (f, "*no geometry*");
		fputc ('\n', f);
	}

	void serialize (gzFile f);
};

/*===========================================================================
 * Name:	IndexedFaceSet
 * Purpose:	Represents a VRML IndexedFaceSet node, i.e. list of triangles.
 */
class IndexedFaceSet : public Node {
public:
	float color[4];
	bool color_specified;
	Point **points;
	int n_points;
	Triangle **triangles;
	int n_triangles;
	int points_size;	// # pointers allocated
	int triangles_size;	// # pointers allocated

	double minx, maxx, miny, maxy, minz, maxz;

	bool force_green;	// for cross section
	bool doing_cross_section;

	/*===================================================================
	 * Name:	ensure_tiny_triangles 
	 * Purpose:	Enforced a maximum triangle size.
	 */
	void ensure_tiny_triangles (double maximum);

	/*===================================================================
	 * Name:	IndexedFaceSet
	 * Purpose:	Sets up IndexedFaceSet, creates point & triangle arrays.
	 */
	IndexedFaceSet () :
		force_green(false), doing_cross_section(false),
		color_specified(false)
	{
		type = "IndexedFaceSet";
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.0f;

#define IFS_INITIAL_NPOINTS 300
#define IFS_INITIAL_NTRIANGLES 600
		n_points = 0;
		points_size = IFS_INITIAL_NPOINTS;
		points = (Point**) malloc (sizeof(Point*)*IFS_INITIAL_NPOINTS);
		// points = new Point*[IFS_INITIAL_NPOINTS];

		n_triangles = 0;
		triangles_size = IFS_INITIAL_NTRIANGLES;
		triangles = (Triangle**) malloc (sizeof (Triangle*) *IFS_INITIAL_NTRIANGLES);
		// triangles = new Triangle*[IFS_INITIAL_NTRIANGLES];
		
		next = NULL;
		children = NULL;
		
		minx = miny = minz = 1E6f;
		maxx = maxy = maxz = -1E6f;

		total_allocated += sizeof(IndexedFaceSet);
		total_allocated += sizeof(Point*) * IFS_INITIAL_NPOINTS;
		total_allocated += sizeof(Triangle*) * IFS_INITIAL_NTRIANGLES;
	}

	/*===================================================================
	 * Name:	~IndexedFaceSet
	 * Purpose:	Carefully deallocates object to avoid memory leaks.
	 */
	~IndexedFaceSet() {
		int i;
		for(i=0; i<n_points; i++)
			delete points[i];
		for(i=0; i<n_triangles; i++)
			delete triangles[i];

		total_allocated -= sizeof(Point*) * points_size;
		total_allocated -= sizeof(Triangle*) * triangles_size;
		total_allocated -= sizeof(IndexedFaceSet);

		free (points);	// Cannot delete[] since these used w/realloc().
		free (triangles);

		if (children)
			delete children;
		children = NULL;
		last_child = NULL;
		if (next)
			delete next;
		next = NULL;
		// name is not a copy.
		// type is a const.
	}


	/*===================================================================
	 * Name:	smooth_faces
	 * Purpose:	Performs smoothing of faces of this indexed face set.
	 */
	void smooth_faces () 
	{
		int i;

		printf ("Smoothing %d triangles...\n", n_triangles );

		//----------------------------------------
		// Clear out the triangle normals arrays.
		//
		for (i=0; i < n_triangles; i++) {
			Triangle *t = triangles[i];
			Point_init_triangle_normals_array (t->p1);
			Point_init_triangle_normals_array (t->p2);
			Point_init_triangle_normals_array (t->p3);
		}

		//----------------------------------------
		// For each triangle, copy its normal to
		// the triangle normals array of each of 
		// its points.
		//
		for (i=0; i < n_triangles; i++) {
			Triangle *t = triangles[i];
			Point *p1 = t->p1;
			Point *p2 = t->p2;
			Point *p3 = t->p3;

			if (p1->n_normals_added >= p1->n_normals_from_triangles)
				Point_realloc_triangle_normals_array (p1);
			if (p2->n_normals_added >= p2->n_normals_from_triangles)
				Point_realloc_triangle_normals_array (p2);
			if (p3->n_normals_added >= p3->n_normals_from_triangles)
				Point_realloc_triangle_normals_array (p3);

			// Recall that records are:
			// 0: triangle normal x
			// 1: triangle normal y
			// 2: triangle normal z
			// 3: triangle area
			p1->normals_from_triangles [p1->n_normals_added * 4] = t->normal_vector->x;
			p1->normals_from_triangles [p1->n_normals_added * 4 + 1] = t->normal_vector->y;
			p1->normals_from_triangles [p1->n_normals_added * 4 + 2] = t->normal_vector->z;
			p1->normals_from_triangles [p1->n_normals_added * 4 + 3] = t->area;
			p1->n_normals_added++;

			p2->normals_from_triangles [p2->n_normals_added * 4] = t->normal_vector->x;
			p2->normals_from_triangles [p2->n_normals_added * 4 + 1] = t->normal_vector->y;
			p2->normals_from_triangles [p2->n_normals_added * 4 + 2] = t->normal_vector->z;
			p2->normals_from_triangles [p2->n_normals_added * 4 + 3] = t->area;
			p2->n_normals_added++;

			p3->normals_from_triangles [p3->n_normals_added * 4] = t->normal_vector->x;
			p3->normals_from_triangles [p3->n_normals_added * 4 + 1] = t->normal_vector->y;
			p3->normals_from_triangles [p3->n_normals_added * 4 + 2] = t->normal_vector->z;
			p3->normals_from_triangles [p3->n_normals_added * 4 + 3] = t->area;
			p3->n_normals_added++;
		}

		// Clear crease flags.
		for (i=0; i < n_points; i++) {
			Point *p = points[i];
			p->along_crease = false;
		}

		//----------------------------------------
		// Calculate weighted vertex normal from 
		// all of each vertex's triangles' normals.
		//
		for (i=0; i < n_points; i++) {			
			Point *p = points[i];
			int j;
			double x = 0.f;
			double y = 0.f;
			double z = 0.f;
			double total_area = 0.f;

			//----------------------------------------
			// Get the total area for all triangles
			// connected to this point.
			//
			for (j=0; j < p->n_normals_added; j++) {
				int ix = j * 4;
				total_area += p->normals_from_triangles[ix + 3];
			}

			//----------------------------------------
			// Add the weighted normals from all 
			// triangles. Weight is just fraction of
			// total area of all connected triangles.
			//
			for (j=0; j < p->n_normals_added; j++) {
				int ix = j * 4;
				double weight;
				weight = p->normals_from_triangles[ix + 3]
					/ total_area;
				x += weight * p->normals_from_triangles[ix];
				y += weight * p->normals_from_triangles[ix + 1];
				z += weight * p->normals_from_triangles[ix + 2];
			}

			//----------------------------------------
			// Normalize & store the normal.
			//
			double mag = sqrt (x*x + y*y + z*z);
			p->normal_x = x / mag;
			p->normal_y = y / mag;
			p->normal_z = z / mag;	
		}

		//----------------------------------------
		// Calculate the maximal angular deviation
		// of each vertex normal from its 
		// triangles' normals. This will tell us
		// which points are along a crease.
		//
		int total_normals_rejected = 0;
		for (i=0; i < n_points; i++) {
			Point *p = points[i];
			double max_angle = 0.;
			int j;
				
			for (j=0; j < p->n_normals_added; j++) {	
				double angle, dotproduct;
				int ix = j*4;
				double x = p->normals_from_triangles[ix];
				double y = p->normals_from_triangles[ix + 1];
				double z = p->normals_from_triangles[ix + 2];

				double mag = sqrt (x*x + y*y + z*z);
				x /= mag;
				y /= mag;
				z /= mag;

				dotproduct = p->normal_x * x 
					+ p->normal_y * y 
					+ p->normal_z * z;

				angle = abs (acosf (dotproduct));
// printf ("angle %g degrees, ", 180.f*angle/M_PI);
				if (angle > max_angle) {
					max_angle = angle;
				}
			}

			//----------------------------------------
			// If the maximal angular difference
			// is over 45 degrees (pi/4) then we shan't
			// smooth this point.
			//
			p->valid_vertex_normal = true;

			if (max_angle > (M_PI/4.f)) {
				p->along_crease = true;
			}
			else 
				p->along_crease = false;
		}

		int total_points_along_creases = 0;
		for (i=0; i < n_points; i++) {
			Point *p = points[i];
			if (p->along_crease)
				++total_points_along_creases;
		}
		printf ("Smoothing found that %d points are along creases (total %d) due to large angles.\n", 
			total_points_along_creases,
			n_points);

		//----------------------------------------
		// For each point of each triangle, 
		// determine whether we have enough normals
		// for smoothing. Ideally we want 3, but
		// points along an edge of a non-closed
		// IndexedFaceSet may have less.
		//
		for (i=0; i < n_triangles; i++) {
			Triangle *t = triangles[i];
			Point *p1 = t->p1;
			Point *p2 = t->p2;
			Point *p3 = t->p3;

			// Here we only invalidate point vertices.
			if (p1->n_normals_added < 3)
				p1->valid_vertex_normal = false;
			if (p2->n_normals_added < 3)
				p2->valid_vertex_normal = false;
			if (p3->n_normals_added < 3)
				p3->valid_vertex_normal = false;
		}

		//----------------------------------------
		// Finally, let's clean up.
		//
		for (i=0; i < n_points; i++) {
			Point *p = points[i];
			Point_destroy_triangle_normals_array (p);
		}

	}

	void serialize (gzFile f);

	/*===================================================================
	 * Name:	expand_points
	 * Purpose:	Increases the size of the points array by 2X.
	 */
	void expand_points () 
	{
		Point** newarray;
		int lim = points_size;
		total_allocated -= sizeof(Point*) * points_size; // delete

		points_size = 2*lim;
		unsigned amount = points_size * sizeof(Point*);
printf ("expand_points: new points_size: %d, amount=%u\n", points_size,amount);
fflush (0);
		newarray = (Point**) malloc (amount);
		if (!newarray) {
			fatal ("out of memory!");
		}
		total_allocated += sizeof(Point*) * points_size;

		int i=0;
		while(i<lim) {
			newarray[i]=points[i];
			i++;
		}
		free (points);
		points = newarray;
	}

	/*===================================================================
	 * Name:	expand_triangles
	 * Purpose:	Increases the size of the triangles array by 2X.
	 */
	void expand_triangles () {
		Triangle** newarray;
		unsigned long lim = triangles_size;
		total_allocated -= sizeof(Triangle*) * triangles_size; // delete

		triangles_size = 2*lim;
		newarray = (Triangle**) malloc (triangles_size*sizeof(void*));
		total_allocated += sizeof(Triangle*) * triangles_size;

		unsigned long i=0;
		while(i<lim) {
			newarray[i]=triangles[i];
			i++;
		}
		free (triangles);
		triangles = newarray;
	}

	void express_ball_at (bool cube, double x, double y, double z, double radius, unsigned long color)
	{
		glPushMatrix ();
		glTranslatef (x, y, z);

		GLfloat materialColor[4];
		materialColor [0] = 255==(0xff & (color>>16)) ? 1.f : 0.f;
		materialColor [1] = 255==(0xff & (color>>8)) ? 1.f : 0.f;
		materialColor [2] = 255==(0xff & color) ? 1.f : 0.f;
		materialColor [3] = 0.f; 
		glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
			materialColor);
		glMaterialfv (GL_FRONT, GL_EMISSION, materialColor);
		if (cube) 
			glutSolidCube (radius);
		else
			glutSolidSphere (radius, 20, 20);
		glPopMatrix ();
	}

	/*===================================================================
	 * Name:	express
	 * Purpose:	Expresses all of the triangles as OpenGL data.
	 */
	void express (bool express_siblings) {
		int i;
		ENTRY

		Shape *shape = (Shape*) parent;
		if (shape) 
			shape->express_colors ();
		else 
			force_green = true;

		if (redrawing_for_selection) {
			for (i=0; i < n_triangles; i++) {
				Triangle *t = triangles[i];
#if defined(WIN32) || defined(__APPLE__)
				glLoadName ((GLuint) t);	// XX not 64 bit correct
#endif
				glBegin(GL_TRIANGLES);
				t->express ();
				glEnd ();
			}
		} else {
			if (force_green) {
				GLfloat materialColor[4];
				materialColor [0] = 0.f;
				materialColor [1] = 1.f;
				materialColor [2] = 0.f;
				materialColor [3] = 0.f; // XX
				glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
					materialColor);
			}

			if (!doing_cross_section) {
#define BALL_BUFFER_SIZE (1000)
				int n_balls = 0;
				double ball_x [BALL_BUFFER_SIZE];
				double ball_y [BALL_BUFFER_SIZE];
				double ball_z [BALL_BUFFER_SIZE];

printf ("Rendering %d triangles in IFS\n", n_triangles);
				glBegin(GL_TRIANGLES);
				for (i=0; i < n_triangles; i++) {
					Triangle *t = triangles[i];

#if defined(WIN32) || defined(__APPLE__)
					if (red_dot_stack) {
						bool name_match = false;
						GLuint name;
						name = (GLuint) t;

						red_dot_info *r = red_dot_stack;
						while (r && !name_match) {
							if (name == r->name) {
								name_match = true;
								break;
							} else
								r = r->next;
						}

						// If we encounter a specific 
						// triangle that's been 
						// selected, we need to place
						// a ball in its position.
						//
						if (name_match) {
							double x, y, z;
							t->get_center (x, y, z);
							ball_x [n_balls] = x;
							ball_y [n_balls] = y;
							ball_z [n_balls] = z;
							n_balls++;
						}
					}
#endif
					t->express ();
				}
				glEnd ();
		
				i = 0;
				while (i < n_balls) {
					//------------------------------
					// Create a red ball where user
					// clicked.
					//
					express_ball_at (false, 
						ball_x[i], ball_y[i], 
						ball_z[i], 0.0005f, 0xff0000);

					i++;
				}
				if (n_balls) {
					// Restore the triangles' color.
					//
					Shape *shape = (Shape*) parent;
					shape->express_colors ();
				}
			}
			else
			{
				// Cross section
				//
				GLfloat materialColor[4];
				unsigned long color = force_green ? 0xff00 : 0xff0000;
				materialColor [0] = 255==(0xff & (color>>16)) ? 1.f : 0.f;
				materialColor [1] = 255==(0xff & (color>>8)) ? 1.f : 0.f;
				materialColor [2] = 255==(0xff & color) ? 1.f : 0.f;
				materialColor [3] = 0.f;
				glColor3f (materialColor[0], materialColor[1], materialColor[2]);
				
				for (i=0; i < n_triangles; i++) {		
					Triangle *t = triangles[i];

					glDisable(GL_LIGHTING);
					glBegin (GL_LINES);
					double x, y, z;
					t->get_center (x, y, z);
		
					// float side = 0.0005f;
					// side /= 2.f;
					Point_express (t->p1);
					Point_express (t->p2);
					Point_express (t->p3);
					Point_express (t->p1);
					glEnd ();
					glBegin (GL_TRIANGLES);
					Point_express (t->p1);
					Point_express (t->p2);
					Point_express (t->p3);
					glEnd ();
					glEnable(GL_LIGHTING);
				}
			}
		}
	}

	/*===================================================================
	 * Name:	report_bounds
	 * Purpose:	Reports to caller maximum and minimum coordinates.
	 */
	void report_bounds (int level, bool report_siblings,
				double &minx_, double &maxx_,
				double &miny_, double &maxy_,
				double &minz_, double &maxz_) 
	{
		ENTRY2

		minx_ = minx;
		maxx_ = maxx;
		miny_ = miny;
		maxy_ = maxy;
		minz_ = minz;
		maxz_ = maxz;
#if 0
		printf ("IndexedFaceSet BOUNDS: %g %g, %g %g, %g %g\n",
			minx,maxx,miny,maxy,minz,maxz);
#endif
		REPORT2
	}
	
	/*===================================================================
	 * Name:	dump
	 * Purpose:	Diagnostic dump.
	 */
	void dump (FILE *f) {
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf(f, "%s (%d points, %d triangles, bounds [%g,%g] [%g,%g] [%g,%g]\n", 
			type, n_points, n_triangles, minx,maxx, miny,maxy, minz,maxz);
	}
};


/*===========================================================================
 * Name:	IndexedLineSet
 * Purpose:	Represents a VRML IndexedLineSet node, i.e. list of lines.
 * Note:	Only one color supported.
 */
class IndexedLineSet : public Node {
public:
	bool color_per_vertex;
	float color[4];
	Point **points;
	float *colors; // organized as r,g,b,r,g,b,...
	int n_points;
	int n_colors;
	int points_size;
	int colors_size;
	PolyLine *polyline;
	PolyLine *last_poly;
	int n_segments;
	double minx, maxx, miny, maxy, minz, maxz;

	IndexedLineSet() {
		type = "IndexedLineSet";
		color_per_vertex = false;
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.0f;
		polyline = NULL;
		last_poly = NULL;
		n_segments = 0;

		n_colors = 0;
#define ILS_INITIAL_NCOLORS 16
		colors_size = ILS_INITIAL_NCOLORS;
		colors = new float[ILS_INITIAL_NCOLORS];

		children = NULL;
		next = NULL;

		minx = miny = minz = 1E6f;
		maxx = maxy = maxz = -1E6f;

		n_points = 0;
		points_size = IFS_INITIAL_NPOINTS;
		points = new Point*[IFS_INITIAL_NPOINTS];

		total_allocated += sizeof(IndexedFaceSet);
		total_allocated += sizeof(Point*) * IFS_INITIAL_NPOINTS;
		total_allocated += sizeof(float) * ILS_INITIAL_NCOLORS;
	}

	void serialize (gzFile f);

	/*===================================================================
	 * Name:	report_bounds
	 * Purpose:	Reports to caller maximum and minimum coordinates.
	 */
	void report_bounds (int level, bool report_siblings,
				double &minx_, double &maxx_,
				double &miny_, double &maxy_,
				double &minz_, double &maxz_) 
	{
		ENTRY2
		minx_ = minx;
		maxx_ = maxx;
		miny_ = miny;
		maxy_ = maxy;
		minz_ = minz;
		maxz_ = maxz;
		REPORT2
	}

	/*===================================================================
	 * Name:	express
	 * Purpose:	Express the lines in the form of OpenGL calls.
	 */
	void express (bool express_siblings) {
		ENTRY

#ifndef ORTHOCAST
		if (!color_per_vertex) {
			glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
			glBegin(GL_LINE_STRIP);

			polyline->express (color_per_vertex);

			glEnd ();
		} else {
			PolyLine *pl = polyline;
			Point *prev = NULL;

			while (pl) {
				Point *p = pl->point;
				if (prev) {
					GLfloat materialColor[4];
					GLfloat materialSpecular[4];
					GLfloat materialEmission[4];
					int i;
					for (i=0; i<3; i++) {
						materialColor[i] = using_custom_colors ? user_fg[i] : color[i];
						materialSpecular[i] = 0 * user_specularity;
						materialEmission[i] = 1.0f;
					}
					materialSpecular[3] = 1.f;
					materialEmission[3] = 1.f;
					materialColor[3] = 1.f;
					glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materialColor);
					glMaterialfv (GL_FRONT, GL_SPECULAR, materialSpecular);
					glMaterialfv (GL_FRONT, GL_EMISSION, materialEmission);
					glMaterialf (GL_FRONT, GL_SHININESS, 127.f);

					glBegin (GL_LINES);
					glVertex3d (prev->x, prev->y, prev->z);
					glVertex3d (p->x, p->y, p->z);
					glEnd ();
				}
				prev = p;
				pl = pl->next;
			}
		}
#endif
	}

	/*===================================================================
	 * Name:	dump
	 * Purpose:	Diagnostic dump.
	 */
	void dump (FILE *f) {
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf (f, "%s \n", type);
	}

	/*===================================================================
	 * Name:	expand_points
	 * Purpose:	Increases the size of the points array by 2X.
	 */
	void expand_points () {
		Point** newarray;
		unsigned long lim = points_size;
		total_allocated -= sizeof(Point*) * points_size;

		points_size = 2*lim;
		newarray = new Point*[points_size];
		total_allocated += sizeof(Point*) * points_size;

		unsigned long i=0;
		while(i<lim) {
			newarray[i]=points[i];
			i++;
		}
		delete[] points;
		points = newarray;
	}

	/*===================================================================
	 * Name:	expand_colors
	 * Purpose:	Increases the size of the colors array by 2X.
	 */
	void expand_colors () {
		float* newarray;
		int lim = colors_size;
		total_allocated -= sizeof(float) * colors_size;

		colors_size = 2*lim;
		newarray = new float[colors_size];
		total_allocated += sizeof(float) * colors_size;

		int i=0;
		while(i<lim) {
			newarray[i]=colors[i];
			i++;
		}
		delete[] colors;
		colors = newarray;
	}

};


/*===========================================================================
 * Name:	Cylinder
 * Purpose:	Represents a VRML Cylinder node.
 */
class Cylinder : public Node {
public:
	bool bottom, top, side;
	double radius;
	double height;

	Cylinder() {
		type = "Cylinder";
		bottom = top = side = true;
		radius = 0.0f;
		height = 0.0f;
		next = NULL;
		children = NULL;

		total_allocated += sizeof(Cylinder);
	}
};

/*===========================================================================
 * Name:	Box
 * Purpose:	Represents a VRML Box node.
 */
class Box : public Node {
public:
	double size;

	Box() {
		type = "Box";
		size = 0.0f;
		next = NULL;
		children = NULL;

		total_allocated += sizeof(Box);
	}
};

/*===========================================================================
 * Name:	Text
 * Purpose:	Represents a VRML Text node.
 */
class Text : public Node {
public:
	char *text;

	Text() {
		type = "Text";
		text = NULL;
		next = NULL;
		children = NULL;

		total_allocated += sizeof(Text);
	}

	void serialize (gzFile );

	void dump (FILE *f)
	{
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf (f, "%s (%s = '%s')\n", 
			type, 
			name? name : "(no name)", 
			text);
	}

	/*===================================================================
	 * Name:	report_bounds
	 * Purpose:	Reports to caller maximum and minimum coordinates.
	 */
	void report_bounds (int level, bool report_siblings,
				double &minx, double &maxx,
				double &miny, double &maxy,
				double &minz, double &maxz) 
	{
		ENTRY2

		minx = miny = minz = 1E6f;
		maxx = maxy = maxy = -1E6f;
	}
};

/*===========================================================================
 * Name:	Cone
 * Purpose:	Represents a VRML Cone node.
 */
class Cone : public Node {
public:
	double radius;
	double height;

	Cone () :
		radius(0.f), height(0.f)
	{
		type = "Cone";
		next = NULL;
		children = NULL;

		total_allocated += sizeof(Cone);
	}

	/*===================================================================
	 * Name:	express
	 * Purpose:	Expresses Sphere using GLUT.
	 */
	void express (bool express_siblings) {
		ENTRY
		glutSolidCone (radius, height, 100, 1);
	}

};

/*===========================================================================
 * Name:	Sphere
 * Purpose:	Represents a VRML Sphere node.
 */
class Sphere : public Node {
public:
	double radius;

	Sphere () {
		type = "Sphere";
		radius = 0.0f;
		next = NULL;
		children = NULL;

		total_allocated += sizeof(Sphere);
	}

	/*===================================================================
	 * Name:	express
	 * Purpose:	Expresses Sphere using GLUT.
	 */
	void express (bool express_siblings) {
		ENTRY
		glutSolidSphere (radius, 50, 10);
	}

	/*===================================================================
	 * Name:	report_bounds
	 * Purpose:	Reports to caller maximum and minimum coordinates.
	 */
	void report_bounds (int level, bool report_siblings,
				double &minx, double &maxx,
				double &miny, double &maxy,
				double &minz, double &maxz) 
	{
		ENTRY2
		minx = -radius;
		maxx = radius;
		miny = -radius;
		maxy = radius;
		minz = -radius;
		maxz = radius;
		REPORT2
	}

	/*===================================================================
	 * Name:	dump
	 * Purpose:	Diagnostic dump.
	 */
	void dump (FILE *f) {
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf (f, "%s %s radius %g\n", type, name? name : "", radius);
	}
};

/*===========================================================================
 * Name:	Replicated
 * Purpose:	Represents a replicated VRML node, i.e. result of USE command.
 */
class Replicated : public Node {
public:
	Node *original; // For clarity, we don't use the 'children' datum.

	/*===================================================================
	 * Name:	Replicated
	 * Purpose:	Creates a replication of named node.
	 */
	Replicated ()
	{
		original = NULL;
	}

	void serialize (gzFile f);

	/*===================================================================
	 * Name:	Replicated
	 * Purpose:	Creates a replication of named node.
	 */
	Replicated (Model *m, char *name) {
		ASSERT_NONZERO(m,"model")
		ASSERT_NONZERO(name,"name")
		//----------

		type = "Replicated (USE)";
		original = m->lookup_node_by_name (name);
		if (!original) {
			char tmp[250];
			sprintf (tmp, "Unknown objected '%s' referred to in USE clause.", name);
			warning(tmp);
		}
		children = NULL;
		next = NULL;

		total_allocated += sizeof(Replicated);
	}

	/*===================================================================
	 * Name:	express
	 * Purpose:	Expresses the object in OpenGL terms.
	 */
	void express (bool express_siblings) {
		ENTRY
		if (original)
			original->express (false);
		if (express_siblings && next)
			next->express (true);
	}

	/*===================================================================
	 * Name:	report_bounds
	 * Purpose:	Reports to caller maximum and minimum coordinates.
	 */
	void report_bounds (int level, bool report_siblings,
				double &minx, double &maxx,
				double &miny, double &maxy,
				double &minz, double &maxz) 
	{
		ENTRY2
			
		double x0, x1, y0, y1, z0, z1;
		x0 = y0 = z0 = 1E6f;
		x1 = y1 = z1 = -1E6f;

		if (original) {
			original->report_bounds (level+1, false, 
					x0, x1, y0, y1, z0, z1);
		}

		if (next && report_siblings) {
			double minx2, maxx2, miny2, maxy2, minz2, maxz2;
			minx2 = miny2 = minz2 = 1E6f;
			maxx2 = maxy2 = maxz2 = -1E6f;

			next->report_bounds (level, true, minx2, maxx2, miny2, maxy2, minz2, maxz2);
			if (minx2 < x0)
				x0 = minx2;
			if (miny2 < y0)
				y0 = miny2;
			if (minz2 < z0)
				z0 = minz2;

			if (maxx2 > x1)
				x1 = maxx2;
			if (maxy2 > y1)
				y1 = maxy2;
			if (maxz2 > z1)
				z1 = maxz2;
		}

		minx = x0;
		miny = y0;
		minz = z0;
		maxx = x1;
		maxy = y1;
		maxz = z1;
		REPORT2
	}

	/*===================================================================
	 * Name:	dump
	 * Purpose:	Diagnostic dump, tells what the pointed-to object is.
	 */
	void dump (FILE *f) {
		ASSERT_NONZERO(f,"file")
		//----------

		fprintf (f, "%s %s: ", type, name? name : "");
		if (original)
			original->dump (f);
		else
			fprintf (f, "NULL");
		fputc('\n', f);
	}
};


#endif /* _MAXILLA_H */
