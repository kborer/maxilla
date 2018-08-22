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

#ifndef _PARENT_H
#define _PARENT_H

void model_parse_worldinfo (Model *, const InputWord *w);
int model_parse_material (Model *, Appearance *appearance, const InputWord *w);
void model_parse_use (Model *, Node *, const InputWord *w, char *);;
void model_parse_appearance (Model *, Shape *, const InputWord *w);
void model_parse_sphere (Model *, Node *, const InputWord *w, char *);
void model_parse_cylinder (Model *, Node *, const InputWord *w, char *);
void model_parse_box (Model *, Node *, const InputWord *w, char *);
void model_parse_text (Model *, Node *, const InputWord *w, char *);
void model_parse_cone (Model *, Node *, const InputWord *w, char *);
void model_parse_indexedlineset (Model *, Node *, const InputWord *w, char *);
void model_parse_indexedfaceset (Model *, Node *, const InputWord *w, char *);
void model_parse_geometry (Model *, Shape *, const InputWord *w, char *);
void model_parse_shape (Model *, Node *, const InputWord *w, char *);
int model_parse_children (Model *, Node *, const InputWord *w);
void model_parse_switch (Model *, Node *, const InputWord *w, char* );
void model_parse_transform (Model *, Node *, const InputWord *w, char *);
void model_parse_separator (Model *, Node *, const InputWord *w, char *);
void model_parse_group (Model *, Node *, const InputWord *w, char *);
void model_parse_viewpoint (Model *, const InputWord *w);
Model * vrml_parser (const InputWord *w);

#endif

