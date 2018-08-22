/*=============================================================================
// Maxilla, an OpenGL-based program for viewing dentistry-related VRML & STL.
// Copyright (C) 2008-2010 by Zack T Smith and Ortho Cast Inc.
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
//============================================================================*/

#ifdef WIN32
	#include <windows.h>
	#define _USE_MATH_DEFINES
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "defs.h"

// zlib.h is read in maxilla.h 

#ifdef WIN32
#include "stdafx.h"
#endif

#include "quat.h"
#include "maxilla.h"

extern "C" {
#include "BMP.h"
}

//---------------------------------------------------------------------------
// Name:	model_parse_worldinfo
// Purpose:	Parser for WorldInfo construct.
//---------------------------------------------------------------------------
void
model_parse_worldinfo (Model *m, const InputWord *w)
{
	w = w->next;
	w = w->children;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(w,"inputword")
	//----------

	while (w) {
		char *s = w->str;
		if (!strcmp("title", s)) {
			w = w->next;
			m->title = w->str;
		}
		else if (!strcmp("info", s)) {
			w = w->next;
		}
		w = w->next;
	}
}

//---------------------------------------------------------------------------
// Name:	model_parse_material
// Purpose:	Parser for material construct, that stores these data
//		in the Shape object.
// Returns:	# of words for caller to skip.
//---------------------------------------------------------------------------
int
model_parse_material (Model *m, Appearance *appearance, const InputWord *w)
{
	int nwords = 3; // material Material {
	char *s;
	char *name=NULL;
	Material *material = NULL;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	s = w->str;

	if (!strcmp(s, "DEF")) {
		w = w->next;
		name = w->str;
		material = new Material ();
		m->add_name_mapping (name, material);
		appearance->material = material;
		w = w->next;
		s = w->str;
		nwords += 2;
	}

	if (strcmp(s, "Material")) {
		warning ("VRML material construct lacks Material{ }.");
	}
	w = w->next;
	w = w->children;

	if (!material) {
		material = new Material ();
		appearance->material = material;
	}

	while (w) {
		s = w->str;
		if (!strcmp("shininess", s)) {
			float tmp;
			w = w->next;
			tmp = w->value;
			if (tmp < 0.0f)
				tmp = 0.0f;
			if (tmp > 100.0f)
				tmp = 100.0f;
			material->shininess = tmp;
		}
		else if (!strcmp("diffuseColor", s)) {
			w = w->next;
			material->diffuseColor[0] = (GLfloat) w->value; 
			w = w->next;
			material->diffuseColor[1] = (GLfloat) w->value; 
			w = w->next;
			material->diffuseColor[2] = (GLfloat) w->value;
		} 
		else if (!strcmp("ambientIntensity", s)) {
			w = w->next;
			material->ambientIntensity = w->value; 
		}
		else if (!strcmp("specularColor", s)) {
			w = w->next;
			material->specularColor[0] = (GLfloat) w->value; 
			w = w->next;
			material->specularColor[1] = (GLfloat) w->value; 
			w = w->next;
			material->specularColor[2] = (GLfloat) w->value;
		}
		else if (!strcmp("emissiveColor", s)) {
			w = w->next;
			material->emissiveColor[0] = (GLfloat) w->value; 
			w = w->next;
			material->emissiveColor[1] = (GLfloat) w->value; 
			w = w->next;
			material->emissiveColor[2] = (GLfloat) w->value;
		}
		w = w->next;
	}

	return nwords;
}

void model_parse_use (Model *m, Node *parent, const InputWord *w, char *name);

//---------------------------------------------------------------------------
// Name:	model_parse_appearance
// Purpose:	Parser for appearance construct.
//---------------------------------------------------------------------------
void
model_parse_appearance (Model *m, Shape *parent, const InputWord *w)
{
	char *name = NULL;
	char *s;
	Appearance *appearance = NULL;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	s = w->str;
	if (!strcmp (s, "DEF")) {
		w = w->next;
		name = w->str;
		appearance = new Appearance ();
		m->add_name_mapping (name, appearance);
		w = w->next;
		s = w->str;
	}
	if (!strcmp(s, "USE")) { // e.g. appearance USE Felt
		w = w->next;
		s = w->str;
		Node *n = m->lookup_node_by_name (s);
		if (!strcmp (n->type, "Appearance"))
			parent->appearance = (Appearance*) n;
		else {
			char tmp[1000];
#ifdef WIN32
			sprintf_s (tmp,999, "Name %s was expected to refer to Appearance, but is %s.",
				s, n->type);
#else
			sprintf (tmp,"Name %s was expected to refer to Appearance, but is %s.",
				s, n->type);
#endif
			warning (tmp);
		}
		return;
	}
	if (strcmp (s, "Appearance")) {
		warning ("VRML appearance construct missing Appearance{ }.");
		warning (s);
		return;
	}

	// If we're not creating a named Appearance,
	// create an unnamed one.
	//
	if (!appearance) {
		appearance = new Appearance ();
	}
	if (!parent->appearance)
		parent->appearance = appearance;
	
	w = w->next;
	w = w->children;

	while (w) {
		s = w->str;
		if (!strcmp (s, "material")) {
			if (model_parse_material (m, appearance, w) > 3) {
				w = w->next;
				w = w->next;
			}
			w = w->next;
			w = w->next;
		} else 
		if (!strcmp (s, "texture")) {
			printf ("Ignoring texture.\n");
			w = w->next;
			w = w->next;
		} else 
		if (!strcmp (s, "textureTransform")) {
			printf ("Ignoring textureTransform.\n");
			w = w->next;
			w = w->next;
		} else {
			warning ("Unknown node type in Appearance.");
			warning(s);
		}
		w = w->next;

	}
}

//---------------------------------------------------------------------------
// Name:	model_parse_use
// Purpose:	Parser for the USE construct.
//---------------------------------------------------------------------------
void
model_parse_use (Model *m, Node *parent, const InputWord *w, char *name)
{
	Replicated *r;
	char *use_name;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	use_name = w->str;
	r = new Replicated (m, use_name);
	r->parent = parent;
	parent->add_child(r);
	if (name)
		m->add_name_mapping (name, r->original);
}

//---------------------------------------------------------------------------
// Name:	model_parse_sphere
// Purpose:	Parser for Sphere construct (child of Shape or Separator).
//---------------------------------------------------------------------------
void
model_parse_sphere (Model *m, Node *parent, const InputWord *w, char *name)
{
	Sphere *sphere;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	w = w->children;

	sphere = new Sphere ();
	sphere->parent = parent;
	if (!strcmp (parent->type, "Shape") && parent->children)
		warning ("Shape already has geometry but additional defined.");
	parent->add_child (sphere);

	if (name)
		m->add_name_mapping (name, sphere);

	while (w) {
		char *s = w->str;
		if (!strcmp(s, "radius")) {
			w = w->next;
			sphere->radius = w->value;
		}
		w = w->next;
	}
}

//---------------------------------------------------------------------------
// Name:	model_parse_cylinder
// Purpose:	Parser for Cylinder construct (child of Shape, Separator).
//---------------------------------------------------------------------------
void
model_parse_cylinder (Model *m, Node *parent, const InputWord *w, char *name)
{
	Cylinder *cylinder;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	w = w->children;

	cylinder = new Cylinder();
	cylinder->parent = parent;
	if (!strcmp (parent->type, "Cylinder"))
		warning ("Shape already has geometry but additional defined.");
	parent->add_child (cylinder);

	if (name)
		m->add_name_mapping (name, cylinder);

	while (w) {
		char *s = w->str;
		if (!strcmp(s, "radius")) {
			w = w->next;
			cylinder->radius = w->value;
		}
		else if (!strcmp(s, "height")) {
			w = w->next;
			cylinder->height = w->value;
		}
		else if (!strcmp(s, "top")) {
			w = w->next;
			cylinder->top = tolower(w->str[0]) == 't';
		}
		else if (!strcmp(s, "bottom")) {
			w = w->next;
			cylinder->top = tolower(w->str[0]) == 't';
		}
		else if (!strcmp(s, "side")) {
			w = w->next;
			cylinder->top = tolower(w->str[0]) == 't';
		}
		w = w->next;
	}
}

//---------------------------------------------------------------------------
// Name:	model_parse_box
// Purpose:	Parser for Box construct (child of Shape or Separator).
//---------------------------------------------------------------------------
void
model_parse_box (Model *m, Node *parent, const InputWord *w, char *name)
{
	Box *box;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	w = w->children;

	box = new Box();
	box->parent = parent;
	if (!strcmp (parent->type, "Shape") && parent->children)
		warning ("Shape already has geometry but additional defined.");
	parent->add_child (box);

	if (name)
		m->add_name_mapping (name, box);

	while (w) {
		char *s = w->str;
		if (!strcmp(s, "size")) {
			w = w->next;
			box->size = w->value;
		}
		w = w->next;
	}
}


//---------------------------------------------------------------------------
// Name:	model_parse_text
// Purpose:	Parser for Text construct (child of Shape or Separator).
//---------------------------------------------------------------------------
void
model_parse_text (Model *m, Node *parent, const InputWord *w, char *name)
{
	Text *text;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	w = w->children;

	text = new Text ();
	text->parent = parent;
	if (!strcmp (parent->type, "Shape") && parent->children)
		warning ("Shape already has geometry but additional defined.");
	parent->add_child (text);

	if (name)
		m->add_name_mapping (name, text);

	text->name = name;
	text->text = "";

	while (w) {
		char *s = w->str;
		if (!strcmp(s, "string")) {

			w = w->next;
			if (!w)	// Sometimes string is followed by nothing.
				break;

			text->text = w->str;

			// Dentistry-specific strings.
			if (name) {
				if (!strncmp (name, "PatientFirstName", 16))
					m->patient_firstname = w->str;
				else if (!strncmp (name, "PatientLastName", 15))
					m->patient_lastname = w->str;
				else if (!strncmp (name, "Age", 3))
					m->patient_birthdate = w->str;
				else if (!strncmp (name, "CaseNumber", 10))
					m->case_number = w->str;
				else if (!strncmp (name, "CaseDate", 8))
					m->case_date = w->str;
				else if (!strncmp (name, "ControlNumber", 13))
					m->control_number = w->str;
				else if (!strncmp (name, "TopAndBottomAligned", 19))
				{
					m->isTopAndBottomAligned = !strncmp (w->str, "true", 4);
				}				
			}
		}
		w = w->next;
	}
}


//---------------------------------------------------------------------------
// Name:	model_parse_cone
// Purpose:	Parser for Cone construct (child of Shape or Separator).
//---------------------------------------------------------------------------
void
model_parse_cone (Model *m, Node *parent, const InputWord *w, char *name)
{
	Cone *cone;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	w = w->children;

	cone = new Cone ();
	cone->parent = parent;
	if (!strcmp (parent->type, "Shape") && parent->children)
		warning ("Shape already has geometry but additional defined.");
	parent->add_child (cone);

	if (name)
		m->add_name_mapping (name, cone);

	while (w) {
		char *s = w->str;
		if (!strcmp (s, "bottomRadius")) {
			w = w->next; 
			if (w->str[0] == '#')
				cone->radius = w->value;
			else
				; // XX
		}
		else if (!strcmp (s, "height")) {
			w = w->next; 
			if (w->str[0] == '#')
				cone->height = w->value;
			else 
				; // XX
		}
		w = w->next;
	}
}


//---------------------------------------------------------------------------
// Name:	model_parse_indexedlineset
// Purpose:	Parser for IndexedLineSet (child of Shape or Separator).
//---------------------------------------------------------------------------
void
model_parse_indexedlineset (Model *m, Node *parent, const InputWord *w, char *name)
{
	IndexedLineSet *ils = new IndexedLineSet ();
	char *name2 = NULL;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	w = w->children;

	while (w) {
		char *s = w->str;
		if (!strcmp("color", s)) {
			w = w->next; 		
			if (strcmp(w->str, "Color")) {
				warning ("VRML IndexedLineSet construct has problematic Color section.");
				delete ils;
				return;
			}
			w = w->next;
			if (w && w->children) {
				InputWord *w2 = w->children;
				if (strcmp (w2->str, "color")) {
					warning ("VRML IndexedLineSet construct has problematic color section.");
					delete ils;
					return;
				}
				w2 = w2->next;
				w2 = w2->children;
				ils->n_colors = 0;
				while (w2) {
					if (w2->str[0] == '#')
						ils->colors [ils->n_colors++] = 
							w2->value;
					if (ils->n_colors >= ils->colors_size)
						ils->expand_colors ();
					w2 = w2->next;
				}
				printf ("IndexedLineSet: read %d color values\n", ils->n_colors);
			}
		}
		else if (!strcmp("colorPerVertex", s)) {
			w = w->next; 
			if (tolower(w->str[0]) == 't')
				ils->color_per_vertex = true;
		}
		else if (!strcmp ("coord", s)) {
			w = w->next;
			if (!strcmp(w->str, "DEF")) {
				w = w->next;
				name2 = w->str;
				w = w->next;
			}
			if (strcmp(w->str, "Coordinate")) {
				warning ("VRML IndexedLineSet construct has problematic coord section.");
				delete ils;
				return;
			}
			w = w->next;

			// Let's read the data here rather than in another function.
			InputWord *w2 = w->children;
			s = w2->str;
			if (strcmp("point", s)) {
				warning ("VRML IndexedLineSet has problematic point list.");
				delete ils;
				return;
			}
			w2 = w2->next;
			w2 = w2->children;

			// Fetch point coordinates in sets of 3 floats.
			float values[3];
			int total = 0;
			while (w2) {
				char ch = *w2->str;
				if (ch == ',') {
					w2 = w2->next;
					continue;
				}
				if (ch == '#' || ch=='-' || isdigit (ch)) {
					values[total++] = ch=='#' ? w2->value
						: (float) atof (w2->str);
					w2 = w2->next;

					if (total >= 3) {
						Point *p = Point_new (values[0],
							values[1], values[2]);
						total = 0;

						if (ils->n_points >= 
						    ils->points_size)
							ils->expand_points();
						ils->points[ils->n_points++]=p;
					}
				} else {
					printf ("Problematic IndexedLineSet datum %s\n", w2->str);
					w2 = w2->next;
				}

			}
//			printf ("Got %d points\n", ils->n_points);
		}
		else if (!strcmp ("coordIndex", s)) {
			w = w->next;

			// Each datum that we read is an index into 
			// the points array.
			//
			InputWord *w2 = w->children;
			int value;
			int total_read = 0;
			ils->polyline = NULL;
			ils->last_poly = NULL;
			while (w2) {
				char ch = *w2->str;
				if (ch == ',') {
					w2 = w2->next;
					continue;
				}
				else
				if (ch == '#' || ch=='-' || isdigit (ch)) {
					value = ch=='#' ? ((int) w2->value) 
							: atoi(w2->str);
					w2 = w2->next;

					if (value<0 || value >= ils->n_points) {
						warning("VRML IndexedLineSet has invalid coordIndex index value (s).");
						delete ils;
						return;
					}

					// The value is just an index into the
					// array of points. So now we obtain the
					// point and make a polyline segment 
					// out of it.
					//
					Point *p = ils->points[value];
					PolyLine *pl = new PolyLine (p);

					if (p->x < ils->minx)
						ils->minx = p->x;
					if (p->x > ils->maxx)
						ils->maxx = p->x;
					if (p->y < ils->miny)
						ils->miny = p->y;
					if (p->y > ils->maxy)
						ils->maxy = p->y;
					if (p->z < ils->minz)
						ils->minz = p->z;
					if (p->z > ils->maxz)
						ils->maxz = p->z;

					if (!ils->polyline)
						ils->polyline = pl;
					else 
						ils->last_poly->next = pl;
					ils->last_poly = pl;

					if (ils->color_per_vertex) {
						int i;
						for (i=0; i<3; i++) 
							pl->color[i] = ils->colors[total_read*3+i];
						pl->color[3] = 1.0f;
					}
					total_read++;

				} else {
					printf ("Problematic IndexedLineSet array value: '%s'\n", w2->str);
					w2=w2->next;
				}
			}
//			printf ("Got %d triangles\n", ils->n_triangles);
		} else {
			printf ("Artifact in IndexedLinesSet: %s\n", w->str);
		}
		w = w->next;
	}

	// If parent is a shape we set our node as its geometry.
	ils->parent = parent;
	if (!strcmp(parent->type, "Shape") && parent->children)
		warning ("Shape already has geometry but additional defined.");
	parent->add_child (ils);
	if (name)
		m->add_name_mapping(name,ils);
}


class IndexedFaceSet;

//---------------------------------------------------------------------------
// Name:	model_parse_indexedfaceset
// Purpose:	Parser for IndexedFaceSet (child of Shape or Separator).
//---------------------------------------------------------------------------
void
model_parse_indexedfaceset (Model *m, Node *parent, const InputWord *w, char *name)
{
	IndexedFaceSet *ifs = new IndexedFaceSet ();
	char *name2 = NULL;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	w = w->children;

	ifs->maxx = ifs->maxy = ifs->maxz = -999999;
	ifs->minx = ifs->miny = ifs->minz = 999999;

	while (w) {
		char *s = w->str;
		if (!strcmp("solid", s)) {
			w = w->next;
		}
		else if (!strcmp ("coord", s)) {
			w = w->next;
			if (!strcmp(w->str, "DEF")) {
				w = w->next;
				name2 = w->str;
				w = w->next;
			}
			if (strcmp(w->str, "Coordinate")) {
				warning ("VRML IndexedFaceSet construct has problematic coord section.");
				delete ifs;
				return;
			}
			w = w->next;

			// Let's read the data here rather than in another function.
			InputWord *w2 = w->children;
			s = w2->str;
			if (strcmp("point", s)) {
				warning ("VRML IndexedFaceSet has problematic point list.");
				delete ifs;
				return;
			}
			w2 = w2->next;
			w2 = w2->children;

			// Fetch point coordinates in sets of 3 floats.
			double values[3];
			int total = 0;
			while (w2) {
				char ch = *w2->str;
				if (ch == ',') {
					w2 = w2->next;
					continue;
				}
				if (ch == '#' || ch=='-' || isdigit (ch)) {
					values[total++] = ch=='#' ? w2->value : (double) atof (w2->str);
					w2 = w2->next;

					if (total >= 3) {
						Point *p = Point_new (values[0], values[1], values[2]);
						total = 0;

						if (ifs->n_points >= ifs->points_size)
							ifs->expand_points();
						ifs->points[ifs->n_points++] = p;
					}
				} else {
					printf ("Problematic IndexedFaceSet datum %s\n", w2->str);
					w2 = w2->next;
				}

			}
//			printf ("Got %d points\n", ifs->n_points);
		}
		else if (!strcmp ("coordIndex", s)) {
			w = w->next;

			// Each four data that we read will represent one triangle.
			InputWord *w2 = w->children;
			int values[4];
			int total_read = 0;
			while (w2) {
				char ch = *w2->str;
				if (ch == ',') {
					w2 = w2->next;
					continue;
				}
				else
				if (ch == '#' || ch=='-' || isdigit (ch)) {
					values[total_read++] = ch=='#' ? ((int) w2->value) : atoi(w2->str);
					w2 = w2->next;
					if (total_read >= 4) {
						total_read = 0;

						if (values[3] != -1) {
							warning("VRML IndexedFaceSet has invalid coordIndex data.");
							delete ifs;
							return;
						}
						if (values[0] < 0 || values[0] >= ifs->n_points ||
						    values[1] < 0 || values[1] >= ifs->n_points ||
						    values[2] < 0 || values[2] >= ifs->n_points) {
								warning("VRML IndexedFaceSet has invalid coordIndex index value (s).");
								delete ifs;
								return;
						}
						Point *p1 = ifs->points[values[0]];
						Point *p2 = ifs->points[values[1]];
						Point *p3 = ifs->points[values[2]];

						Triangle *t = new Triangle (p1, p2, p3);
						t->model = m;

						t->indices [0] = values [0];
						t->indices [1] = values [1];
						t->indices [2] = values [2];

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

						if (ifs->n_triangles >= ifs->triangles_size)
							ifs->expand_triangles();
						ifs->triangles[ifs->n_triangles++] = t;
					}
				} else {
					printf ("Problematic array value: '%s'\n", w2->str);
					w2=w2->next;
				}
			}
//			printf ("Got %d triangles\n", ifs->n_triangles);
		}
		w = w->next;
	}	

	// If parent is a shape we set our node as its geometry.
	ifs->parent = parent;
	if (!strcmp(parent->type, "Shape") && parent->children)
		warning ("Shape already has geometry but additional defined.");
	parent->add_child (ifs);
	if (name)
		m->add_name_mapping(name,ifs);

	ifs->ensure_tiny_triangles (0.0000010f); // formerly 6
}

//---------------------------------------------------------------------------
// Name:	model_parse_geometry
// Purpose:	Parser for geometry construct.
//---------------------------------------------------------------------------
void
model_parse_geometry (Model *m, Shape *parent, const InputWord *w, char *name)
{
	char *s;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	s = w->str;
	if (!strcmp(s, "DEF")) {
		w = w->next;
		w = w->next;
		s = w->str;
	}

	if (!strcmp(s, "USE")) {
		Replicated *r;
		w = w->next;
		s = w->str;
		r = new Replicated(m, s);
		parent->add_geometry (r);
		return;
	}

	if (!strcmp(s, "IndexedFaceSet")) {
		model_parse_indexedfaceset (m, parent, w, name);
	}
	else if (!strcmp(s, "IndexedLineSet")) {
		model_parse_indexedlineset (m, parent, w, name);
	}
	else if (!strcmp(s, "Sphere")) {
		model_parse_sphere (m, parent, w, name);
	}
	else if (!strcmp(s, "Cone")) {
		model_parse_cone (m, parent, w, name);
	}
	else if (!strcmp(s, "Cylinder")) {
		model_parse_cylinder (m, parent, w, name);
	}
	else if (!strcmp(s, "Box")) {
		model_parse_box (m, parent, w, name);
	}
	else if (!strcmp(s, "Text")) {
		model_parse_text (m, parent, w, name);
	}
	else
		printf ("Geometry of type %s ignored.\n", s);
}

//---------------------------------------------------------------------------
// Name:	model_parse_shape
// Purpose:	Parser for shape construct.
//---------------------------------------------------------------------------
void
model_parse_shape (Model *m, Node *parent, const InputWord *w, char *name)
{
	Shape *shape;
	char *name2 = NULL;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	shape = new Shape ();
	shape->parent = parent;
	parent->add_child(shape);
	if (name)
		m->add_name_mapping (name, shape);

	// Move to children.
	w = w->next;
	w = w->children;

	while (w) {
		char *s = w->str;
		if (!strcmp("geometry", s)) {
			InputWord *w2 = w->next;
			if (w2 && !strcmp("DEF", w2->str)) {
				w2 = w2->next;
				name2 = w2->str;
				w2 = w2->next;
				w2 = w2->next;
			}
			model_parse_geometry (m, shape, w, name2);
			name2 = NULL;
			w = w2;
		}
		else if (!strcmp("appearance", s)) {
			model_parse_appearance (m, shape, w);
			w = w->next;
		}
		w = w->next;
	}
}

void model_parse_group (Model *m, Node *parent, const InputWord *w, char *name);
void model_parse_transform (Model *m, Node *parent, const InputWord *w, char *name);
void model_parse_switch (Model *m, Node *parent, const InputWord *w, char *name);

//---------------------------------------------------------------------------
// Name:	model_parse_children
// Purpose:	Parser for Transform's and Group's children array.
// Returns:	Number of InputWords that the caller should skip.
//---------------------------------------------------------------------------
int
model_parse_children (Model *m, Node *parent, const InputWord *w)
{
	char *name = NULL;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	// Locates the children.
	// This can be an array inside brackets or it can a single node.
	//
	w = w->next;
	if (!strcmp(w->str, "DEF")) {
		w = w->next;
		name = w->str;
#if 0
printf ("In parse_children, found name %s\n", name);
#endif
		w = w->next;
	}	
	//
	if (!strcmp(w->str, "Shape")) {
		model_parse_shape (m, parent, w, name);
		return 2;
	}	
	else if (!strcmp(w->str, "Group")) {
		model_parse_group (m, parent, w, name);
		return 2;
	}
	else if (!strcmp(w->str, "Transform")) {
		model_parse_transform (m, parent, w, name);
		return 2;
	}
	else if (!strcmp(w->str, "Switch")) {
		model_parse_switch (m, parent, w, name);
		return 2;
	}
	else if (!strcmp(w->str, "Inline")) {
		warning("VRML Inline construct not supported.");
		return 2;
	}

	if ('[' != *w->str) {
		printf ("In parse_children, next node != array: %s\n", w->str);
	}

	w = w->children;

	while (w) {
		char *s = w->str;
// printf ("In parse_children, string is %s, parent = %s, name = %s\n", parent->type, s,name);
		if (!strcmp("DEF", s)) {
			w = w->next;
			name = w->str;
			w = w->next;
			continue;
		}
		else if (!strcmp("Shape", s)) {
			model_parse_shape (m, parent, w, name);
			name = NULL;
			w = w->next;
		}
		else if (!strcmp("USE", s)) {
			model_parse_use (m, parent, w, name);
			name = NULL;
			w = w->next;
		}
		else if (!strcmp("Inline", s)) {
			warning ("VRML Inline construct found: not supported.");
			name = NULL;
			w = w->next;
		}
		else if (!strcmp("Transform", s)) {
			model_parse_transform (m, parent, w, name);
			name = NULL;
			w = w->next;
		}		
		else if (!strcmp("Switch", s)) {
			model_parse_switch (m, parent, w, name);
			name = NULL;
			w = w->next;
		}	
		else if (!strcmp("Group", s)) {
			model_parse_group (m, parent, w, name);
			name = NULL;
			w = w->next;
		}
		w = w->next;
	}
	return 1;
}



//---------------------------------------------------------------------------
// Name:	model_parse_switch
// Purpose:	Parser for Switch.
//---------------------------------------------------------------------------
void
model_parse_switch (Model *m, Node *parent, const InputWord *w, char* name)
{
	Switch *swtch;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	swtch = new Switch();
	swtch->name = name;
	if (name)
		m->add_name_mapping (name, swtch);
	swtch->parent = parent;
	parent->add_child(swtch);

	

	w = w->next;
	w = w->children;

	while (w) {
		char *s = w->str;
		if (!strcmp(s, "choice")) {
			if (2 == model_parse_children (m, swtch, w))
				w = w->next;
			w = w->next;

			// Count the total choices.
			int count = 0;
			Node *n = swtch->children;
			while (n) {
				count++;
				n = n->next;
			}
			swtch->total_choices = count;
		}
		else if (!strcmp(s, "whichChoice")) {
			int choice;
			w = w->next;
			choice = (int) w->value;
			swtch->which = choice;
		}
		w = w->next;
	}

	if (name && !strncmp(name, "switchNode", 10))
		m->register_switch (swtch);
}

//---------------------------------------------------------------------------
// Name:	model_parse_transform
// Purpose:	Parser for transform construct.
//---------------------------------------------------------------------------
void
model_parse_transform (Model *m, Node *parent, const InputWord *w, char *name)
{
	// char *name2 = NULL;
	Transform *transform;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	transform = new Transform();
	transform->name = name;
	if (name)
		m->add_name_mapping (name, transform);
	transform->parent = parent;
	parent->add_child(transform);

	w = w->next;
	w = w->children;

	while (w) {
		char *s = w->str;
		if (!strcmp("rotation", s)) {	
			w = w->next;
			transform->rotate_x = w->value; w = w->next;
			transform->rotate_y = w->value; w = w->next;
			transform->rotate_z = w->value; w = w->next;
			transform->rotate_angle = w->value;
			if (transform->op_index >= TRANSFORM_MAX_OPS)
				warning ("Too many translate, rotate, or scale operations inside Transform construct.");
			else
				transform->operations[transform->op_index++] = 
					Transform::ROTATE;
		}
		else if (!strcmp("translation", s)) {	
			w = w->next;
			transform->translate_x = w->value; w = w->next;
			transform->translate_y = w->value; w = w->next;
			transform->translate_z = w->value; 
// printf ("Found translation in %s: %g %g %g\n", name, transform->translate_x, transform->translate_y, transform->translate_z);
			if (transform->op_index >= TRANSFORM_MAX_OPS)
				warning ("Too many translate, rotate, or scale operations inside Transform construct.");
			else
				transform->operations[transform->op_index++] = 
						Transform::TRANSLATE;
		}
		else if (!strcmp("scale", s)) {
			w = w->next;
			transform->scale_x = w->value; w = w->next;
			transform->scale_y = w->value; w = w->next;
			transform->scale_z = w->value; 
			if (transform->op_index >= TRANSFORM_MAX_OPS)
				warning ("Too many translate, rotate, or scale operations inside Transform construct.");
			else
				transform->operations[transform->op_index++] = 
					Transform::SCALE;
		}
		else if (!strcmp("center", s)) {
			w = w->next;
			transform->center_x = w->value; 
			w = w->next;
			transform->center_y = w->value; 
			w = w->next;
			transform->center_z = w->value; 

			// Not used.
		}
		else if (!strcmp("children", s)) {
			if (2 == model_parse_children (m, transform, w))
				w = w->next;
			w = w->next;
		}
		w = w->next;
	}
}

//---------------------------------------------------------------------------
// Name:	model_parse_separator
// Purpose:	Parser for Separator construct.
//---------------------------------------------------------------------------
void
model_parse_separator (Model *m, Node *parent, const InputWord *w, char *name)
{
	char *name2 = NULL;
	Separator *sep;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	sep = new Separator();
	sep->name = name;
	if (name)
		m->add_name_mapping (name, sep);
	sep->parent = parent;
	parent->add_child(sep);

	w = w->next;
	w = w->children;
	while (w) {
		char *s = w->str;
		if (!strcmp("DEF", s)) {
			w = w->next;
			name2 = w->str;
			w = w->next;
			continue;
		}
		else if (!strcmp("Material", s)) {	
			w = w->next;
		}
		else if (!strcmp("Sphere", s)) {	
			model_parse_sphere (m, sep, w, name2);
			w = w->next;
		}
		else if (!strcmp("IndexedFaceSet", s)) {
			model_parse_indexedfaceset (m, sep, w, name2);
			w = w->next;
		}
#if 0
		else if (!strcmp("center", s)) {
			w = w->next;
			sep->center_x = w->value; w = w->next;
			sep->center_y = w->value; w = w->next;
			sep->center_z = w->value; 
		}
		else if (!strcmp("children", s)) {
			if (2 == model_parse_children (m, sep, w))
				w = w->next;
			w = w->next;
		}
#endif
		w = w->next;
	}
}

//---------------------------------------------------------------------------
// Name:	model_parse_group
// Purpose:	Parser for group construct.
//---------------------------------------------------------------------------
void
model_parse_group (Model *m, Node *parent, const InputWord *w, char *name)
{
	char *name2 = NULL;
	Group *group;

	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(parent,"parent-node")
	ASSERT_NONZERO(w,"inputword")
	//----------

	group = new Group();
	group->name = name;
	if (name)
		m->add_name_mapping (name, group);
	group->parent = parent;
	parent->add_child(group);

#ifdef ORTHOCAST
	// This is part of a kludge that's required because
	// the DST input files lack a fourth Switch choice
	// for an "open" or occlusal view of dentistry files.
	//
	if (name && !strncmp (name, "TopAndBottom", 12)) {
		m->top_and_bottom_node = group;
	}
#endif

	w = w->next;
	w = w->children;
	while (w) {
		char *s = w->str;
		if (!strcmp(s, "children")) {
			if (2 == model_parse_children (m, group, w))
				w = w->next;
			w = w->next;
		}
		else if (!strcmp(s, "bboxCenter")) {
			float x = w->value; w = w->next;
			float y = w->value; w = w->next;
			float z = w->value;
		}
		else if (!strcmp(s, "bboxSize")) {
			float wid = w->value; w = w->next;
			float h = w->value; w = w->next;
			float depth = w->value;
		}
		w = w->next;
	}
}

//---------------------------------------------------------------------------
// Name:	model_parse_viewpoint
// Purpose:	Parser for viewpoint construct.
// Note:	This program supports only one viewpoint at this time.
//---------------------------------------------------------------------------
void
model_parse_viewpoint (Model *m, const InputWord *w)
{
	ASSERT_NONZERO(m,"model")
	ASSERT_NONZERO(w,"inputword")
	//----------

	w = w->next;
	w = w->children;
	while (w) {
		char *s = w->str;
		if (!strcmp("position", s)) {
			w = w->next;
			cc_main.viewpoint_x = w->value; w = w->next;
			cc_main.viewpoint_y = w->value; w = w->next;
			cc_main.viewpoint_z = w->value;
		}
		else if (!strcmp("orientation", s)) {
			w = w->next;
			cc_main.orientation_x = w->value; w = w->next;
			cc_main.orientation_y = w->value; w = w->next;
			cc_main.orientation_z = w->value; w = w->next;
			float foo = w->value;
		}
		else if (!strcmp("fieldOfView", s)) {
			float tmp;
			w = w->next;
			tmp = w->value;
			if (tmp > 0.1f) {
				cc_main.field_of_view = 360.0f * tmp / (float) M_PI;
			}
		}
		else if (!strcmp("jump", s) ||
				!strcmp("description", s) ||
				!strcmp("isBound", s) ||
				!strcmp("set_bind", s) ||
				!strcmp("bindTime", s)) {
			w = w->next;  // skip param
		}
		w = w->next;
	}
}


//---------------------------------------------------------------------------
// Name:	model_parser
// Purpose:	Parser for VRML files.
//---------------------------------------------------------------------------
Model *
vrml_parser (const InputWord *w)
{
	Model *m;
	Node *node;
	char *name = NULL; // used with DEF.

	ASSERT_NONZERO(w,"inputword")
	//----------

	m = new Model();
	node = new Node ();
	m->nodes = node;

	while (w) {
		char *str = w->str;
// printf ("Word is %s\n", str);
		if (!strcmp("Viewpoint", str)) {
			model_parse_viewpoint (m, w);
			w = w->next; 
		}
		else if (!strcmp("Shape", str)) {
			model_parse_shape (m, node, w, name);
			name = NULL;
			w = w->next;
		}
		else if (!strcmp("Transform", str)) {			
// printf ("Got transform %s\n", name);
			model_parse_transform (m, node, w, name);
			name = NULL;
			w = w->next;
		}
		else if (!strcmp("Group", str)) {
			model_parse_group (m, node, w, name);
			name = NULL;
			w = w->next;
		}
		else if (!strcmp("Separator", str)) {
			model_parse_separator (m, node, w, name);
			name = NULL;
			w = w->next;
		}
		else if (!strcmp("Switch", str)) {
			model_parse_switch (m, node, w, name);
			name = NULL;
			w = w->next;
		}
		else if (!strcmp("USE", str)) {
			model_parse_use (m, node, w, name); // [DEF foo] USE bar
			name = NULL;
			w = w->next;
		}
		else if (!strcmp("Background", str)) {
			w = w->next;
			InputWord *c = w->children->next;
			m->background_provided = true;
			m->bg[0] = c->value; c = c->next;
			m->bg[1] = c->value; c = c->next;
			m->bg[2] = c->value;
		}
		else if (!strcmp("PROTO", str)) {
			warning ("VRML contains a PROTO construct, which is not supported.");
			w = w->next;
			w = w->next;
			w = w->next;
		}
		else if (!strcmp("ROUTE", str) ||
				!strcmp("field", str)) {
			w = w->next;	// Skip 3.
			w = w->next;
			w = w->next;
		}
		else if (!strcmp("EXTERNPROTO", str) ||
				!strcmp("eventIn", str) ||
				!strcmp("eventOut", str)) {
			w = w->next;	// Skip 2
			w = w->next;
		}
		else if (!strcmp("WorldInfo", str)) {
			model_parse_worldinfo (m, w);
			w = w->next;
		}
		else if (!strcmp("DirectionalLight", str)) {
			printf ("Found VRML DirectionalLight\n");
			w = w->next;
		}
		else if (!strcmp("DEF", str)) {
			w = w->next;
			name = w->str;
			w = w->next;
			continue;
		}
		else if (!strcmp("TimeSensor", str) || 
				!strcmp("CoordinateInterpolator", str) || 
				!strcmp("PositionInterpolator", str) ||
				!strcmp("NavigationInfo", str)) {
			w = w->next;
		} else
			printf ("Found VRML '%s'\n", str);
		
		w = w->next;
	}
	return m;
}

