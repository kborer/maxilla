//============================================================================= 
// Maxilla, an OpenGL-based program for viewing dentistry-related VRML & STL.
// Copyright (C) 2008-2013 by Zack T Smith and Ortho Cast Inc.
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
//============================================================================
// NOTICE NOTICE NOTICE
//
// Please don't add any use of multiple inheritance or templates to Maxilla.
// Please don't add too many layers of derived classes i.e. lasagna code.
// Please don't use any nested templates.
// Let's keep this program small, understandable & clean.
//
//----------------------------------------------------------------------------
// #define DEBUG
// #define DISABLE_DIAGNOSTICS

#ifdef WIN32
#define strcasecmp stricmp
#endif

// To do:
// *	Automatic positioning of camera to ensure all of model is in frame.
// *	Add exception logic for VRML syntax errors.
// *	Ought to draw Cylinder.
// *	Finish parsing of Separator.
// *	Add support for ASCII STL files.

#ifdef WIN32
	#include <windows.h>
	#define _USE_MATH_DEFINES
#else
	#include <sys/time.h> 	// gettimeofday
#endif

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "defs.h"
#include "Point.h"

// zlib.h is read in maxilla.h 

#ifdef WIN32
#include "stdafx.h"
#endif

#include "quat.h"
#include "maxilla.h"
#include "parser.h"
#include "stl.h"

extern "C" {
#include "PDF.h"
#include "BMP.h"
}

#include "STLRenderContext.h"

extern long millisecond_time ();

static bool will_need_to_add_position_structures = false;

#ifdef __APPLE__
extern int macosx_file_open (char *, int);
extern int macosx_file_save (char *, int);
extern int macosx_message_box (char *, char *);
extern void macosx_color_chooser (void);
#endif

#ifdef __linux__
extern int linux_file_open (char *, int);
extern int linux_file_save (char *, int);
extern int linux_message_box (char *, char *);
#endif

#ifndef WIN32
int stricmp (char *a, char *b) 
{
	return strcasecmp (a, b);
}
#endif

//-------------------------------------------
// The various views offered for examining
// a model.
//
enum {
	VIEW_FRONT=1, // anterior
	VIEW_BACK=8,
	VIEW_LEFT=2,
	VIEW_RIGHT=3,
	VIEW_TOP=4,
	VIEW_X_CROSS_SECTION=5,
	VIEW_Y_CROSS_SECTION=6,
	VIEW_Z_CROSS_SECTION=7,
};

//-------------------------------------------
// User-specified color scheme.
//
bool using_custom_colors;
GLfloat user_bg[4];
GLfloat user_fg[4];
GLfloat user_shininess;
GLfloat user_specularity;
GLfloat user_emissivity;

//-------------------------------------------
// Delayed execution.
// This is necessary because GLUT won't allow
// removal of a subwindow from a callback
// until it returns. And it appears that some
// more primitive implementations of OpenGL
// won't update the GUI at all until we've
// returned from our callback. 
//
enum {
	OP_PRINTING_OBTAIN_PATH = 'o',
	OP_PRINTING = 'm',
	OP_OPEN = 'O',
	OP_COLORS = 'C',
	OP_PRINTING_BEGIN = 'b',
	OP_SINGLEVIEW_PRINTING = 's',
	OP_PRINTING_END = 'e',
	OP_MULTIVIEW_PRINTING_1 = '1',
	OP_MULTIVIEW_PRINTING_2 = '2',
	OP_MULTIVIEW_PRINTING_3 = '3',
	OP_MULTIVIEW_PRINTING_4 = '4',
	OP_MULTIVIEW_PRINTING_5 = '5',
	OP_MULTIVIEW_PRINTING_6 = '6',
	OP_TEST = 't',
	OP_INVOKE_ACROBAT = 'a',
	OP_EXIT = 'x',
	OP_SAVE_AS = 'S',
	OP_OPEN_STL = 'L',
	OP_INVOKE_PDF2JPG = 'P',
	OP_INVOKE_STLEXPORT = 'X',
#if 0
	OP_BOLTON_PRINTING_1 = 'B',
	OP_BOLTON_PRINTING_2 = 'z',
#endif
};

#define MAX_OPS (32)
static char ops [MAX_OPS];
static int ops_params [MAX_OPS];
static long op_duration = 0;	// Time to wait before running operation.
static long op_t0 = 0;
static bool op_param = false;
static BMP* op_bmp = NULL;
static char op_path [PATH_MAX];
static bool ops_pause = false;
static const int MAX_TEMP_PDF_FILES = 50;

void
ops_init ()
{
	memset ((char*) ops, 0, sizeof(ops));
}

int
ops_get ()
{
	return ops[0];
}

int
ops_get_param ()
{

	return ops_params [0];
}

void
ops_add (int op, int param)
{
	int i = MAX_OPS-1;
	while (i >= 0 && !ops[i])
		i--;
	i++;
	ops [i] = op;
	ops_params [i] = param;
	
	op_t0 = millisecond_time ();
	op_duration = 500;
}

void
ops_next ()
{
	int i;
	for (i=0; i<MAX_OPS-1; i++) {
		ops[i] = ops[i+1];
		ops_params [i] = ops_params [i+1];
	}
	ops[MAX_OPS-1] = 0;
	ops_params [MAX_OPS-1] = 0;
}

//-------------------------------------------
// The main 3D window.
//
static int main_window = -1;
static int screendump_counter = 0;

//-------------------------------------------
// The secondary 3D window used for measuring.
//
GLUI_Translation *bolton_widget_zoom = NULL;
GLUI_Translation *bolton_widget_translation = NULL;
static int secondary_window = -1;
#define N_MEASUREMENT_WIDGETS (32)
static char *measurement_names [N_MEASUREMENT_WIDGETS] = {
	"UL1", "UL2", "UL3", "UL4", "UL5", "UL6", "UL7", "UL8",
	"UR1", "UR2", "UR3", "UR4", "UR5", "UR6", "UR7", "UR8",
	"LL1", "LL2", "LL3", "LL4", "LL5", "LL6", "LL7", "LL8",
	"LR1", "LR2", "LR3", "LR4", "LR5", "LR6", "LR7", "LR8",
};
static GLUI_EditText *secondary_measurement_widgets [N_MEASUREMENT_WIDGETS];
static int secondary_current_measurement = -1;
static float secondary_measurements [N_MEASUREMENT_WIDGETS];
static GLUI_StaticText *widget_mandibular_sum = NULL;
static GLUI_StaticText *widget_maxillary_sum = NULL;
static GLUI_StaticText *widget_ratio = NULL;
static GLUI_Button *widget_button_bolton_start [N_MEASUREMENT_WIDGETS];

//-------------------------------------------
// Model characteristics
//
static bool doing_autocenter = true;
static char which_cross_section = 0;	// can be 'x', 'y', 'z' or 0 for none
static int current_view = -1;
#ifdef ORTHOCAST
static bool showing_maxilla = true;	// dentistry-specific
static bool showing_mandible = true;	// dentistry-specific
static bool have_occlusal = false; // kludge
static int occlusal_index = 0; // index into list
static bool showing_occlusal = false;
#define METERS_PER_INCH (0.0254)
static float occlusal_space = METERS_PER_INCH / 2.f;
static Transform *occlusal_top = NULL;
static Transform *occlusal_bottom = NULL;
static float top_depth, bottom_depth;
#endif

//----------------------------------------
// Manual spacing (movement of mandible)
//
static bool doing_manual_spacing = false;
static bool have_manual_spacing = false; // kludge 2.0
static int manual_spacing_index = 0; // index into list
static Transform *manual_spacing_top = NULL;
static Transform *manual_spacing_bottom = NULL;
static float manual_spacing_viewpoints [6];
static float manual_spacing_rotation[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,	
	0.0f, 0.0f, 0.0f, 1.0f
};
static GLUI_Checkbox *widget_manual_spacing = NULL;	
bool did_manual_spacing; // Set only if something really changed.

//-------------------------------------------
// Projection characteristics
//
static bool using_preset_fieldofview = true;
static float xy_aspect = 1.f;
static float subwindow_aspect = 1.f;
static float ortho_z_adjust = 0.f;

//-------------------------------------------
// Viewport characteristics
//
static int usable_x, usable_y;
static int usable_width, usable_height;

//-------------------------------------------
// Mouse state.
//
static int mouse_x = 0;
static int mouse_y = 0;
static bool mouse_doing_click = true;
static bool mouse_dragging = false;
static int mouse_button = -1;  // none
static unsigned long mouse_t0 = 0; // milliseconds

//-------------------------------------------
// Multiview
//
static bool doing_multiview = false;
static GLUI_Checkbox *widget_multiview = NULL;
#define N_SUBWINDOWS (6)
static short subwindows [N_SUBWINDOWS];
static short subwindows_height, subwindows_width;
enum {
	SHOW_MAXILLA = 900,
	SHOW_MANDIBLE = 901,
	SHOW_BOTH = 902,
	SHOW_NOTHING = 903
};

//-------------------------------------------
// Light positions & brightnesses.
//
#define DEFAULT_ORTHOGRAPHIC_SCALE_FACTOR (0.15f)
#define DEFAULT_SCALE_FACTOR (0.15f)

static float ambient_brightness = 0.60f;
static WhiteLight light0 (true);

CameraCharacteristics cc_main; // also used in parser.cpp.
// Secondary window uses cc_main.
static CameraCharacteristics cc_subwindows [N_SUBWINDOWS];

// Live variables that are updated by GLUI when
// widgets are altered:
//
static float zoom_live_var = 0.f;
static float pan_live_vars [3] = { 0.f, 0.f, 0.f };

//-------------------------------------------
// Input files specified on the command-line.
//
static InputFile *input_file = NULL;

//-------------------------------------------
// The 3D model itself.
//
static Model *model = NULL;
bool redrawing_for_selection;
static bool showing_bolton = false;
#define SELECTION_BUFFER_SIZE 512
GLuint selection_buffer[SELECTION_BUFFER_SIZE];
static int selection_buffer_count = 0;
bool doing_smooth_shading = true;

float arbitrary_x_translate;
float arbitrary_y_translate;

//-------------------------------------------
// Z-axis measuring lines
//
bool using_measuring_lines;
static GLfloat line1_value = 0.;
static GLfloat line2_value = 0.;
static GLfloat initial_line1_value = 0.; // tare value
static GLfloat initial_line2_value = 0.; // tare value
static GLfloat recentest_line1_value = 0.; // tare value
static GLfloat recentest_line2_value = 0.; // tare value
static GLfloat our_starting_line1_value = 0.;
static GLfloat our_starting_line2_value = 0.;
static double measuring_line_1 = 0.005;
static double measuring_line_2 = -0.005;
int which_measuring_line_direction; // 0 = first, 1 = second (depends on axis)
static GLUI_Checkbox *widget_line_measuring = NULL;
static GLUI_Translation *widget_line0 = NULL;
static GLUI_Translation *widget_line1 = NULL;
static GLUI_Button *button_overbite = NULL; // upper button
static GLUI_Button *button_overjet = NULL;  // lower button

//-------------------------------------------
// Orthographic feature.
//
static bool doing_orthographic = true;

//-------------------------------------------
// Secondary window widgets.
//
static GLUI *glui2_left = NULL;
static GLUI *glui2_right= NULL;

//-------------------------------------------
// Main window widgets.
//
static GLUI *glui_right = NULL;
static GLUI *glui_bottom = NULL;

//-------------------------------------------
// Popups
//
static bool popup_active = false;


//-------------------------------------------
// Cut away feature.
//
bool doing_cut_away = false;
int cut_away_axis = 0;
GLfloat cut_away_plane_location = 0.00;
GLdouble cut_away_plane_equation[4] = {0,0,-1,cut_away_plane_location};

//-------------------------------------------
// Translation feature
//
GLfloat translation_pane_location = 0.00;

//-------------------------------------------
// GLUI widgets
//
static GLUI_StaticText *widget_title = NULL;
static GLUI_StaticText *widget_field_of_view = NULL;
static GLUI_Checkbox *widget_ortho = NULL;
static GLUI_Checkbox *widget_colors = NULL;
static GLUI_Checkbox *widget_fieldofview45 = NULL;
static GLUI_Checkbox *widget_autocenter = NULL;
static GLUI_StaticText *widget_filename = NULL;
static GLUI_StaticText *widget_x_angle = NULL;
static GLUI_StaticText *widget_y_angle = NULL;
static GLUI_StaticText *widget_status_line = NULL;
static GLUI_Checkbox *widget_cross_section_x = NULL;
static GLUI_Checkbox *widget_cross_section_y = NULL;
static GLUI_Checkbox *widget_cross_section_z = NULL;
static GLUI_Checkbox *widget_cutaway = NULL;
static GLUI_Translation *widget_cutaway_trans = NULL;
static GLUI_Translation *widget_zoom = NULL;
static GLUI_Translation *widget_translation = NULL;
static GLUI_Button *widget_anterior = NULL;
static GLUI_Button *widget_lingual = NULL;
static GLUI_Button *widget_left = NULL;
static GLUI_Button *widget_right = NULL;

#ifdef ORTHOCAST
// Dentistry-specific:
static GLUI_Checkbox *widget_maxilla = NULL;		
static GLUI_Checkbox *widget_mandible = NULL;		
static GLUI_Button *widget_occlusal = NULL;
static GLUI_Button *widget_occlusal_aligned = NULL;
static GLUI_EditText *widget_patient_last_name = NULL;	
static GLUI_EditText *widget_patient_first_name = NULL;	
static GLUI_EditText *widget_patient_birth_date = NULL;
static GLUI_EditText *widget_case_date = NULL;
static GLUI_EditText *widget_case_number = NULL;
static GLUI_EditText *widget_control_number = NULL;
static GLUI_Translation *widget_relative_translation_x = NULL;
static GLUI_Translation *widget_relative_translation_y = NULL;
static GLUI_Translation *widget_relative_translation_z = NULL;
static GLUI_Rotation *widget_relative_rotation = NULL;
static GLUI_EditText *widget_doctor_name = NULL;
static char doctor_name [PATH_MAX];
#endif

//-------------------------------------------
// Diagnostics.
//
unsigned long total_allocated;
unsigned long time_program_start;

//-------------------------------------------
// Forward declaration
//
void glui_cut_away_axis_callback (const int control);
void glui_line_dir_callback (const int control);
void gui_set_status (char*);
CameraCharacteristics *get_pertinent_cc ();
void handle_resize (const int w, const int h);
void draw_scene ();
void draw_scene_inner (CameraCharacteristics *cc);
void gui_set_status (char *str);
Node* find_node_by_type (Node *node, char *type);
#ifdef ORTHOCAST
void find_maxilla_mandible (IndexedFaceSet* &ifs1, IndexedFaceSet* &ifs2);
#endif
void set_examination_angles (CameraCharacteristics *cc, const float yaw, const float pitch);
void gui_set_status (char*);
void set_secondary_value (int which, float value);
unsigned long determine_clicked_triangle (double &cx, double &cy, double &cz);
void glui_print_callback (const int control);

int serialization_indentation_level;

//-------------------------------------------
// The red dot stack.
//
static bool rotation_locked = false; // When true, red dot can be moved around.
static GLUI_Button *button_lock_rotation = NULL;
static bool doing_measuring = false;
static GLUI_Checkbox *widget_measuring = NULL;
red_dot_info *red_dot_stack;

void
push_dot (red_dot_info *r)
{
	r->next = red_dot_stack;
	red_dot_stack = r;
}

void
drop_dot ()
{
	if (red_dot_stack) {
		red_dot_info *r = red_dot_stack;
		red_dot_stack = r->next;
		delete r;
	}
}

void
redraw_all ()
{
	if (!doing_multiview) {
		if (!showing_bolton)
			glutSetWindow (main_window);
		else
			glutSetWindow (secondary_window);

		glutPostRedisplay ();
	} else {
		for (int i = 0; i < N_SUBWINDOWS; i++) {
			glutSetWindow (subwindows[i]);
			glutPostRedisplay ();
		}
	}
}

//---------------------------------------------------------------------------
// Name:	Callbacks for marker dots.
//---------------------------------------------------------------------------
void
red_dot_remove_last ()
{
	drop_dot ();
	
	redraw_all ();
}

void
red_dot_remove_all ()
{
	while (red_dot_stack)
		drop_dot ();

	redraw_all ();

	gui_set_status ("All marker dots removed.");
}

void
indent (gzFile f)
{
	int i = serialization_indentation_level;
	while (i > 0) {
		gzprintf (f, "    ");
		i--;
	}
}


//---------------------------------------------------------------------------
// Name:	glui_line_measuring_callback
// Purpose:	Handles line checkbox activity.
//---------------------------------------------------------------------------
void 
glui_line_measuring_callback (const int control)
{
	// Choose correct cut away axis.
	//
	cut_away_axis = 1;
	glui_cut_away_axis_callback (0);

	// Set up initial measuring line positions.
	if (model) {
		our_starting_line1_value = model->maxz;
		our_starting_line1_value = model->maxz * .8;
	}
	if (measuring_line_1 == measuring_line_2)
		measuring_line_1 += 0.01;

	//----------------------------------------
	// These values will be overwritten
	// as soon as the user uses the xlation 
	// widget.
	//
	measuring_line_1 = our_starting_line1_value;
	measuring_line_2 = our_starting_line2_value;

	//------------------------------
	// Workaround for GLUI bug of 
	// set_float_val not working.
	// Recalibrate initial value.
	//
	initial_line1_value = recentest_line1_value;
	initial_line2_value = recentest_line2_value;
	recentest_line1_value = 0.;
	recentest_line2_value = 0.;

	//--------------------
	// Show left view.
	//
	CameraCharacteristics *cc = get_pertinent_cc ();
	which_cross_section = 0;
	light0.reset (doing_orthographic ? 'o' : 'n');
	set_examination_angles (cc, 90.f, 0.f);
	glutPostRedisplay ();
}

//---------------------------------------------------------------------------
// Name:	glui_measuring_callback
// Purpose:	Callback function to handle checkbox activity.
//---------------------------------------------------------------------------
void 
glui_measuring_callback (const int control)
{
	if (!model) 
		return;

	doing_measuring = widget_measuring->get_int_val () ? true : false;
	rotation_locked = doing_measuring;
	
	//----------------------------------------
	// Whether starting or stopping measuring, 
	// clear the red dot list.
	//
	red_dot_remove_all ();	// <-- Does redraw.
}

void
clear_manual_spacing ()
{
	int i;
	for (i=0; i<6; i++)
		manual_spacing_viewpoints [i] = 0.f;
	for (i=0; i<16; i++)
		manual_spacing_rotation [i] = 0.f;
	manual_spacing_rotation [0] = 1.f;
	manual_spacing_rotation [5] = 1.f;
	manual_spacing_rotation [10] = 1.f;
	manual_spacing_rotation [15] = 1.f;
}

CameraCharacteristics::CameraCharacteristics () :
	scale_factor (DEFAULT_ORTHOGRAPHIC_SCALE_FACTOR),
	viewpoint_x (0.0f),
	viewpoint_y (0.0f),
	viewpoint_z (1.0f),
	orientation_x (0.0),
	orientation_y (0.0),
	orientation_z (0.0),
	field_of_view (45.f)
{
	int i;
	for (i=0; i<3; i++)
		viewpoints [i] = 0.f;

	clear_manual_spacing ();
	
	starting_rotation.from_euler (0.f, 0.f, 0.f, false);
	combined_rotation.from_euler (0.f, 0.f, 0.f, false);

}

//---------------------------------------------------------------------------
// Name:	get_temp_print_path
// Purpose:	Gets the path for temporary PDFs used for printing.
//          Includes trailing slash so filename can be appended directly
//          to the output path.
//---------------------------------------------------------------------------
bool get_temp_print_path(char* dest, size_t destSize)
{
	memset(dest, 0, destSize);

	char *homedir = getenv ("HOMEPATH");
	if (!homedir)
	{
		homedir = getenv ("HOME");
	
		if (!homedir) 
		{
			return false;
		}
	}

	if (strlen(homedir) > destSize - 4)
	{
		return false;
	}

	#ifndef __APPLE__
		strcpy (dest, "c:");
		strcat (dest, homedir);
		strcat (dest, "\\");
	#else
		strcpy (dest, homedir);
		strcat (dest, "/");
	#endif

	return true;
}

//---------------------------------------------------------------------------
// Name:	get_temp_print_filename
// Purpose:	Gets a filename for a temporary PDF, using the given print
//          number. e.g. maxilla01.pdf, maxilla02.pdf, etc.
//---------------------------------------------------------------------------
void get_temp_print_filename(char* dest, size_t destSize, int printNum)
{
	memset(dest, 0, destSize);
	sprintf (dest, "maxilla%02d.pdf", printNum);
}

//---------------------------------------------------------------------------
// Name:	cleanup_temp_pdfs
// Purpose:	Deletes any temporary PDFs used for printing.
//---------------------------------------------------------------------------
void cleanup_temp_pdfs()
{
	char home_path[MAX_PATH];
	char full_pdf_path[MAX_PATH];
	char filename[30];

	memset(home_path, 0, sizeof(char)*MAX_PATH);
	memset(full_pdf_path, 0, sizeof(char)*MAX_PATH);	
	memset(filename, 0, 30);

	bool gotPath = get_temp_print_path(home_path, sizeof(char)*MAX_PATH);
	if (!gotPath)
		return;

	// Delete all possible temp PDF files.
	for (int printNum = 1; printNum <= MAX_TEMP_PDF_FILES; printNum++)
	{			
		get_temp_print_filename(filename, 30, printNum);
		strcpy (full_pdf_path, home_path);
		strcat (full_pdf_path, filename);		

		// Safety check to ensure file we're about to delete is a PDF.
		int len = strlen (full_pdf_path);
		if (len >= 4 && strcmp (".pdf", full_pdf_path + len - 4) == 0)
		{
			// Delete file, ignoring any error - e.g. file doesn't exist, file is locked.
			remove(full_pdf_path);
		}
	}
}

void 
myexit (int retval)
{
	diag_write ("Program exit");
	exit (retval);
}

// Registered handler for atexit(). This is the only way to capture
// the application exiting if the user clicks the main window [X], 
// because the OpenGL main loop internally calls exit() without providing
// any way to handle it. This is the last function to be called after 
// an exit() is called, right before the program actually exits.
void
atexithandler(void)
{
	cleanup_temp_pdfs();
}

//---------------------------------------------------------------------------
// Name:	fatal_null_pointer
// Purpose:	Exits if parameter is zero.
//---------------------------------------------------------------------------
void
fatal_null_pointer (const char *func, char *param) 
{
	char tmp[200]; 
	sprintf (tmp,"In function %s, param %s was incorrectly NULL.\n",
		func, param); 
#ifdef WIN32
	MessageBoxA (NULL,tmp,0,0); 
	myexit (EXIT_FAILURE);
#endif
#ifdef __APPLE__
	macosx_message_box ("NULL pointer", tmp);
#endif
#ifdef __linux__
	linux_message_box ("NULL pointer", tmp);
#endif
#if !defined(WIN32) && !defined(__APPLE__) && !defined(__linux__)
	fprintf (stderr,tmp);
#endif
	myexit (-1);
}

//---------------------------------------------------------------------------
// Name:	fatal
// Purpose:	Handle a fatal error situation.
//---------------------------------------------------------------------------
void
fatal (const char *s)
{
#ifdef WIN32
	char tmp[250];
	sprintf (tmp, "Fatal error: %s", s);
	fprintf (stderr, "%s\n", tmp);
	MessageBoxA (NULL, tmp, "Fatal error", MB_OK);
	myexit (EXIT_FAILURE);
#endif
#ifdef __APPLE__
	macosx_message_box ("Fatal Error", (char*) s);
#endif
#ifdef __linux__
	linux_message_box ("Fatal Error", (char*) s);
#endif
#if !defined(WIN32) && !defined(__APPLE__) && !defined(__linux__)
	fprintf (stderr, "Fatal error: %s\n", s);
#endif
	myexit (1);
}

//---------------------------------------------------------------------------
// Name:	notice
// Purpose:	Provide the user with some useful information.
//---------------------------------------------------------------------------
void
notice (char *s)
{
#ifdef WIN32
	char tmp[250];
	sprintf (tmp, "Note: %s", s);
	MessageBoxA (NULL, s, "Information", MB_OK);
#endif
#ifdef __APPLE__
	macosx_message_box ("Notice", s);
#endif
#ifdef __linux__
	linux_message_box ("Notice", s);
#endif
#if !defined(WIN32) && !defined(__APPLE__) && !defined(__linux__)
	fprintf (stderr, "%s\n", tmp);
#endif
}

//---------------------------------------------------------------------------
// Name:	warning
// Purpose:	Handle a non-fatal error situation.
//---------------------------------------------------------------------------
void
warning (char *s)
{
#ifdef WIN32
	char tmp[250];
	sprintf (tmp, "Warning: %s", s);
	fprintf (stderr, "%s\n", tmp);
	MessageBoxA (NULL, s, "Warning", MB_OK);
#endif
#ifdef __APPLE__
	macosx_message_box ("Warning", s);
#endif
#ifdef __linux__
	linux_message_box ("Warning", s);
#endif
#if !defined(WIN32) && !defined(__APPLE__) && !defined(__linux__)
	fprintf (stderr, "Warning: %s\n", s);
#endif
}

void
warning_str_int (char *s, int i)
{
	char tmp [256];
	sprintf (tmp, "%s %d", s, i);
	warning (tmp);
}

//---------------------------------------------------------------------------
// Name:	set_configuration_flags
// Purpose:	For a given subwindow, sets the showing_* flags.
//---------------------------------------------------------------------------
void
set_configuration_flags (CameraCharacteristics *cc)
{
	if (doing_multiview || doing_cut_away || doing_manual_spacing || showing_occlusal) {
		showing_maxilla = true;
		showing_mandible = true;
	}
	else
	switch (cc->configuration) {
	case SHOW_MAXILLA:
		showing_maxilla = true;
		showing_mandible = false;
		break;
	case SHOW_MANDIBLE:
		showing_maxilla = false;
		showing_mandible = true;
		break;
	case SHOW_BOTH:
		showing_maxilla = true;
		showing_mandible = true;
		break;
	case SHOW_NOTHING:
		showing_maxilla = false;
		showing_mandible = false;
		break;
	}

	if (!model || !model->main_switch)
		return;

	int new_choice = 3;

	// The whichChoice values in a "DST" file (=.wrl.gz) are always as follows:
	// 0 = top & bottom
	// 1 = bottom (mandible)
	// 2 = top (maxilla)
	// 3 = occlusal view
	// 4 ~= manual spacing

	if (showing_occlusal)
		new_choice = occlusal_index;
	else if (doing_manual_spacing)
		new_choice = manual_spacing_index;
	else if (doing_cut_away)
		new_choice = 0;
	else if (cc->configuration == SHOW_BOTH)
		new_choice = 0;
	else if (cc->configuration == SHOW_MAXILLA)
		new_choice = 2;
	else if (cc->configuration == SHOW_MANDIBLE)
		new_choice = 1;

	Switch *sw = (Switch*) model->main_switch;
	sw->which = new_choice;
	sw->which_node = NULL;
	sw->doing_open_view = showing_occlusal || doing_manual_spacing;
	sw->update_which_node ();

	bool need_recenter = doing_multiview || cc->need_recenter;
	if (need_recenter)
		model->autocenter ();
	cc->need_recenter = false;
}

//---------------------------------------------------------------------------
// Name:	copy
// Purpose:	Copies the visible 3D model from the GL pixel buffer to a 
//		BMP image.
//---------------------------------------------------------------------------
void
copy (BMP *bmp) 
{
	int x = usable_x;
	int y = usable_y;
	int w = usable_width;
	int h = usable_height;
	unsigned long *pixels;
	int num_values = w * h;

	pixels = (unsigned long *) malloc (num_values * sizeof (long));
	if (!pixels) {
		fatal ("Unable to allocate memory.");
	}
	memset (pixels, 0xff, num_values * sizeof (long));

	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	int offset_for_status_bar = 30;
	glReadPixels (usable_x, /*offset_for_status_bar*/ usable_y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	int e = glGetError ();
	if (e) 
		printf ("GL Error %d\n", e);

	int n_nonzero = 0;
	int i, j;

	// Debugging to examine pixels in case of ugly stripe.
	char dump_path [PATH_MAX];
	char *homedir = getenv ("HOMEPATH");
	if (!homedir)
		homedir = getenv ("HOME");
	if (!homedir) {
		puts ("Cannot locate user's home directory.");
		return;
	}
#ifdef __APPLE__
	strcat (dump_path, homedir);
	strcat (dump_path, "/pixels.txt");
#else
	strcpy (dump_path, "c:");
	strcat (dump_path, homedir);
	strcat (dump_path, "\\pixels.txt");
#endif
	FILE *f = NULL; // fopen (dump_path, "wb");

	for (j = 0; j < h; j++) {
		if (f) fprintf (f, "Row %d\n", j);
		for (i = 0; i < w; i++) {
			int ix = j * w + i;
			unsigned long rgb, r, b;
			rgb = pixels [ix];
			if (f) fprintf (f, "%08lx \n", (unsigned long)rgb);
			if ((0xff000000 & rgb) == 0) 
				rgb = 0xffffff;
			else {
				r = rgb & 0xff;
				b = rgb >> 16;
				rgb &= 0xff00;
				b &= 0xff;
				r <<= 16;
				rgb |= r;
				rgb |= b;
			}
			if (rgb)
				n_nonzero++;
			BMP_putpixel (bmp, i, j, rgb);
		}
	}

#if 0
	// Draw some diagnostic lines.
	//
	for (i=0; i<h; i++)
		BMP_putpixel (bmp, i, i, 0xff);
	for (i=0; i<h; i++)
		BMP_putpixel (bmp, w-1-i, h-1-i, 0xff);
	for (i=0; i<w; i++) {
		BMP_putpixel (bmp, i, 0, 0xff);
		BMP_putpixel (bmp, i, h-1, 0xff);
	}
	for (i=0; i<h; i++) {
		BMP_putpixel (bmp, 0, i, 0xff);
		BMP_putpixel (bmp, w-1, i, 0xff);
	}
#endif

	if (f) fclose (f);
	// printf ("n_nonzero %d\n", n_nonzero);
	
	free (pixels);
}

//---------------------------------------------------------------------------
// Name:	calculate_subwindow_center_on_page 
// Purpose:	Calculates the image center in points (72 dpi).
// Note:	Assumes US Letter page.
//---------------------------------------------------------------------------
void
calculate_subwindow_center_on_page (int n, float *x_return, float *y_return)
{
	if (n < 0 || n >= N_SUBWINDOWS)
		return;
	if (!x_return || !y_return)
		return;
	//----------

	float x, y;

#define FULL_PAGE_WIDTH 720.f
#define FULL_PAGE_HEIGHT (7.f * 72.f)
#define FRAME_WIDTH 240.f
#define FRAME_HEIGHT (FULL_PAGE_HEIGHT / 2.f)
#define FRAME_ASPECT (FRAME_WIDTH/FRAME_HEIGHT)
#define STRIP_HEIGHT (36.f)

	x = (float) (n/2);
	x *= FRAME_WIDTH;
	x += 36.f;
	y = (float) (n & 1);
	y *= FRAME_HEIGHT;
	y += 36.f + STRIP_HEIGHT;
	
	*x_return = x + FRAME_WIDTH/2.;
	*y_return = y + FRAME_HEIGHT/2.;
}

//---------------------------------------------------------------------------
// Name:	printing_begin
// Purpose:	Output current view as PDF file.
//---------------------------------------------------------------------------

static double saved_zoom_factor;
static bool saved_using_custom;
static GLfloat saved_user_fg [4], saved_user_bg [4];
static int saved_window_width, saved_window_height;

void 
printing_begin ()
{
	int i;

	if (popup_active) 		
		return;
	if (!strlen (op_path))
		return;

#ifdef WIN32
	_unlink (op_path);
#endif

	

	if (doing_multiview) {
		int i;
		for (i = 0; i < N_SUBWINDOWS; i++) {
			glutSetWindow (subwindows [i]);
			glutHideWindow ();
		}	

		glFlush ();
	} 
		
	glutSetWindow (main_window);
	GLUI_Master.get_viewport_area (&usable_x, &usable_y, &usable_width, &usable_height);
	int num_values = usable_width * usable_height;

	op_bmp = BMP_new (usable_width, usable_height);
	if (!op_bmp) {
		puts ("Out of memory.");
		return;
	}

	int bmp_size = usable_width * usable_height;
	memset (op_bmp->pixels, 0xff, bmp_size*4);

	int len = strlen (op_path);
	if (len >= 4 && strcmp (".pdf", op_path + len - 4))
		strcat (op_path, ".pdf");

	if (!pdf_start ()) {
		gui_set_status ("Unable to write to PDF.");
		if (doing_multiview) {
			// Restore subwindows 
			//
			for (i=0; i < N_SUBWINDOWS; i++) {
				glutSetWindow (subwindows[i]);
				glutShowWindow ();
			}
		}
		ops_init ();
		return;
	}

	//------------------------------
	// Establish printing colors.
	//

	

	saved_zoom_factor = doing_multiview? cc_subwindows[0].scale_factor : cc_main.scale_factor;
	saved_using_custom = using_custom_colors;
	using_custom_colors = true;
	for (i = 0; i < 4; i++) {
		saved_user_fg [i] = user_fg [i];
		saved_user_bg [i] = user_bg [i];
	}
	user_fg[0] = 0.9;
	user_fg[1] = 0.9;
	user_fg[2] = 0.9;
	user_fg[3] = 0;
	user_bg[0] = 1.f;
	user_bg[1] = 1.f;
	user_bg[2] = 1.f;
	user_bg[3] = 0.f;

	//----------------------------------------
	// Establish a zoom factor that will 
	// printing to real-life scale.
	//
#define PDF_ZOOM (DEFAULT_ORTHOGRAPHIC_SCALE_FACTOR * .765f)
#define PDF_ZOOM_MULTIVIEW (DEFAULT_ORTHOGRAPHIC_SCALE_FACTOR * 2.3189189189189189189)

}

//---------------------------------------------------------------------------
// Name:	printing_core
// Purpose:	Output current view as PDF file.
//---------------------------------------------------------------------------

void 
printing_core (int which_subwindow)
{
	int i;

	glutSetWindow (main_window);
	GLUI_Master.get_viewport_area (&usable_x, &usable_y, &usable_width, &usable_height);
	int num_values = usable_width * usable_height;

	//----------------------------------------
	// Redraw each window and copy its pixels
	// into a BMP and then into the PDF.
	//
	if (!doing_multiview) {
		cc_main.scale_factor = PDF_ZOOM;
		glutPostWindowRedisplay (main_window);
		glFlush ();

		draw_scene ();
		draw_scene ();		

		copy (op_bmp);
		pdf_draw_big_image (op_bmp);
	} else {
		glutSetWindow (main_window);
		glutPostWindowRedisplay (main_window);
		glFlush ();
	
		cc_subwindows[which_subwindow].need_recenter = true;

		//----------------------------------------
		// For multiview, the goal is to use
		// most of the 11x8.5" page area except 
		// *	0.5" margins.
		// *	1" strip at the bottom.
		// Organize this area as 3 by 2 images.
		//
		float cx, cy, w, h, aspect;

		aspect = (float) usable_width;
		aspect /= (float) usable_height;

		if (aspect > FRAME_ASPECT) {
			w = FRAME_WIDTH;
			h = w /aspect;
		} else {
			h = FRAME_HEIGHT;
			w = h * aspect;
		}

		static char positions [N_SUBWINDOWS] = {
			1, 3, 0, 2, 5, 4
		};

		i = which_subwindow;

		CameraCharacteristics *cc = &cc_subwindows [i];

		cc->scale_factor = PDF_ZOOM_MULTIVIEW * .98; // Shrink to ensure model is 100% on screen.
		cc->need_recenter = true;
		doing_multiview = false;
		draw_scene_inner (cc);
		draw_scene_inner (cc);
		doing_multiview = true;

		int posn = (int) positions [which_subwindow];
		calculate_subwindow_center_on_page (posn, &cx, &cy);

#if 0
char msg[1000];
sprintf (msg, "Printing subwindow %d, @%g,%g, pixel size %dx%d\n",i,cx,cy,w,h);
warning (msg);
#endif

		//--------------------
		// Clear the BMP.
		//
		int bmp_size = usable_width * usable_height;
		memset (op_bmp->pixels, 0xff, bmp_size*4);

		//--------------------
		// Read pixels into BMP.
		//
		copy (op_bmp);

#if 0
		//--------------------------------------
		// Write images to desktop for testing.
		//
		char path[1000];
		sprintf (path, "c:/users/me/desktop/x%d.bmp", i);
		BMP_write (op_bmp, path);
#endif

		//------------------------------
		// Copy BMP into PDF.
		// Expand about the central point
		// to adjust for shrinking (above).
		//
		pdf_draw_image (op_bmp, cx, cy, w/0.98, h/0.98,  
			usable_width, usable_height);
	}
}

bool isNullOrWhitespace(char* text)
{
	if (!text) return true;
	if (strlen(text) == 0) return true;
	char* s = text;
	for (unsigned int i = 0; i < strlen(text); i++) 
	{
		if(!isspace((int)(*s)))
		{
			return false;
		}
		s++;
	}
	return true;
}

//---------------------------------------------------------------------------
// Name:	printing_end
// Purpose:	Output current view as PDF file.
//---------------------------------------------------------------------------

void
printing_end ()
{
	int i;

	//----------------------------------------
	// Set up the text for the page.
	//
	char practice_name [300];
	char patient_name [200];
	char third [200];
	char fourth [200];
	char fifth [200];
	char sixth [200];

	fourth[0] = 0;
	fifth[0] = 0;
	sixth[0] = 0;

	//-------------
	if (doctor_name [0])
		sprintf (practice_name, "Practice: %s", doctor_name);
	else
		sprintf (practice_name, "Practice: (not specified)");

	//-------------
	bool hasFirstName = !isNullOrWhitespace(model->patient_firstname);
	bool hasLastName = !isNullOrWhitespace(model->patient_lastname);

	if (model && hasFirstName && hasLastName)
		sprintf (patient_name, "Patient: %s, %s",
			model->patient_firstname, model->patient_lastname);
	else
	if (model && !hasFirstName && hasLastName)
		sprintf (patient_name, "Patient: %s",
			model->patient_lastname);
	else
	if (model && hasFirstName && !hasLastName)
		sprintf (patient_name, "Patient: %s",
			model->patient_firstname);
	else
		sprintf (patient_name, "Patient: (no name)");

	//-------------
	if (model && model->patient_birthdate)
		sprintf (third, "Birth Date: %s",
			model->patient_birthdate);
	else
		sprintf (third, "Birth Date: (none)");

	//-------------
	if (model && model->case_number)
		sprintf (fourth, "Case No.: %s",
			model->case_number);
	else
		sprintf (fourth, "Case No.: (none)");

	//-------------
	if (model && model->case_date)
		sprintf (fifth, "Case Date: %s",
			model->case_date);
	else
		sprintf (fifth, "Case Date: (none)");

	//-------------
	if (model && model->control_number)
		sprintf (sixth, "Control No.: %s",
			model->control_number);
	else
		sprintf (sixth, "Control No.: (none)");

	//----------------------------------------
	// Print the text to the PDF.
	//
	char *text [6] = { practice_name, patient_name, third,
			fourth, fifth, sixth };

	int printResult = pdf_end (op_path, text, 6, 8,
			doing_multiview ? BOTTOM : TOPLEFT);

	if (printResult == 1)
	{
		char tmp [PATH_MAX+20];
		sprintf (tmp, "Wrote PDF to %s.\n", op_path);
		gui_set_status (tmp);
	}

	//----------------------------------------
	// Restore colors.
	//
	using_custom_colors = saved_using_custom;
	for (i = 0; i < 4; i++) {
		user_fg [i] = saved_user_fg [i];
		user_bg [i] = saved_user_bg [i];
	}

	glutSetWindow (main_window);
	glutReshapeWindow(saved_window_width, saved_window_height);

	if (!doing_multiview) {
		cc_main.scale_factor = saved_zoom_factor;
	} else {
		for (i = 0; i < N_SUBWINDOWS; i++) {
			CameraCharacteristics *cc = &cc_subwindows [i];
			cc->scale_factor = saved_zoom_factor;
			glutSetWindow (subwindows [i]);
			glutShowWindow ();
		}
	}

	redraw_all ();
	draw_scene ();

	BMP_delete (op_bmp);
	op_bmp = NULL;

	if (printResult == 0)
	{
		char tmp [PATH_MAX+100];
		sprintf (tmp, "Unable to write to '%s'. If this PDF file is open in another program, please close it and try again.\n", op_path);
		warning(tmp);
	}
}


void
invoke_acrobat ()
{
	char cmd [1000];
#ifdef __APPLE__
	sprintf (cmd, "open \"%s\"", op_path);
#else
	sprintf (cmd, "start \"foo\" \"%s\"", op_path);
#endif
	system (cmd);
}


#ifdef WIN32
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

TCHAR* GetThisPath(TCHAR* dest, size_t destSize)
{
    if (!dest) return NULL;
    if (MAX_PATH > destSize) return NULL;

    DWORD length = GetModuleFileName( NULL, dest, destSize );
    PathRemoveFileSpec(dest);
    return dest;
}

#endif

void
invoke_pdf2jpg ()
{		
#ifdef WIN32
	
	TCHAR dest[MAX_PATH];
	GetThisPath(dest, MAX_PATH);
	 
	TCHAR cmd [1000];
	TCHAR params [1000];	
	TCHAR pdfPath [MAX_PATH];

	strcpy(pdfPath, op_path);

	_stprintf(cmd, _T("%s\\convert.exe"), dest);
	_stprintf(params, _T("-colorspace sRGB -verbose -density 150 -trim \"%s\" -quality 100 -background white -flatten \""), op_path);
		
	PathRemoveExtension(op_path);
	strcat (op_path, ".jpg");
	strcat (params, op_path);
	strcat (params, "\"");
		
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = cmd;        
	ShExecInfo.lpParameters = params;   
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_HIDE;
	ShExecInfo.hInstApp = NULL; 

	::SetCursor( LoadCursor( 0, IDC_WAIT ));

	::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	BOOL ok = ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
	::SetCursor( LoadCursor( 0, IDC_ARROW ));

	//HINSTANCE h = ShellExecute(NULL, _T("open"), cmd, params, NULL, SW_NORMAL);

	if(!ok)
	{
		//fail
		TCHAR msg [5000];
		_stprintf(msg, _T("Failed to convert PDF to JPG Error: %d\n\n%s %s"), (int)GetLastError(), cmd, params);
		MessageBox(NULL,msg, NULL, MB_OK);
	}
	else
	{
		char tmp [PATH_MAX];
		sprintf (tmp, "Wrote JPG to %s.\n", op_path);
		gui_set_status (tmp);

		// delete the tmp pdf
		remove(pdfPath);
	}

#else    

	warning ("Not supported");
	
#endif	

}


//---------------------------------------------------------------------------
// Name:	dump_screen_to_bmp
// Purpose:	Output current view as BMP file.
//---------------------------------------------------------------------------
void 
dump_screen_to_bmp ()
{
	GLUI_Master.get_viewport_area (&usable_x, &usable_y, &usable_width, &usable_height);
	int num_values = usable_width * usable_height;
	char path[PATH_MAX];
	char msg[PATH_MAX];
	int i, j;
	unsigned long *pixels;

	pixels = (unsigned long *) malloc (num_values * sizeof (long));
	if (!pixels) {
		fatal ("Unable to allocate memory.");
	}
	memset (pixels, 0, num_values * sizeof (long));

//	glReadBuffer (GL_FRONT);
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	glReadPixels (usable_x, usable_y, usable_width, usable_height, 
		GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	sprintf (path, "c:/windows/temp/maxilla_%03d.bmp", screendump_counter++);
#ifdef WIN32
	_unlink (path);
#endif

	BMP *bmp = BMP_new (usable_width, usable_height);

	int size = usable_width * usable_height;
	memset (bmp->pixels, 0xff, size*4);

	int n_nonzero = 0;
	for (j = 0; j < usable_height; j++) {
		for (i = 0; i < usable_width; i++) {
			int ix = j * usable_width + i;
			unsigned long rgb, r, b;
			rgb = pixels [ix];
			r = rgb & 0xff;
			b = rgb >> 16;
			rgb &= 0xff00;
			b &= 0xff;
			r <<= 16;
			rgb |= r;
			rgb |= b;
			if (rgb)
				n_nonzero++;
			BMP_putpixel (bmp, i, usable_height-j-1, rgb);
		}
	}

	if (1 != BMP_write (bmp, path))
		sprintf (msg, "Failed to write image file %s.", path);
	else
		sprintf (msg, "Screen dumped to %s.", path);
	gui_set_status (msg);

	free (pixels);
	BMP_delete (bmp);
}
 
void
multiview_on ()
{
	int i;
	float ary [3];
	for (i=0; i < 3; i++)
		ary[i] = cc_subwindows[0].viewpoints[i];
	widget_translation->set_float_array_val (ary);

	zoom_live_var = cc_subwindows[0].scale_factor;

#if 0
	//----------------------------------------
	// Former arrangement of views.
	//
	set_examination_angles (&cc_subwindows [0], 90.f, 0.f);
	cc_subwindows[0].configuration = SHOW_BOTH;

	set_examination_angles (&cc_subwindows [1], 0.f, 0.f);
	cc_subwindows[1].configuration = SHOW_BOTH;

	set_examination_angles (&cc_subwindows [2], -90.f, 0.f);
	cc_subwindows[2].configuration = SHOW_BOTH;

	set_examination_angles (&cc_subwindows [3], 180.f, 0.f);
	cc_subwindows[3].configuration = SHOW_BOTH;

#if N_SUBWINDOWS == 6
	cc_subwindows[4].configuration = SHOW_MAXILLA;
	set_examination_angles (&cc_subwindows [4], 0.f, 90.f);

	cc_subwindows[5].configuration = SHOW_MANDIBLE;
	set_examination_angles (&cc_subwindows [5], 0.f, -90.f);
#endif
#else
	//----------------------------------------
	// New arrangement of views.
	//
	// ------------------------------------
	// |           |           |          |
	// |     0     |     1     |    4     |
	// |           |           |          |
	// ------------------------------------
	// |           |           |          |
	// |     2     |     3     |    5     |
	// |           |           |          |
	// ------------------------------------
	set_examination_angles (&cc_subwindows [0], -90.f, 0.f);
	cc_subwindows[0].configuration = SHOW_BOTH;

	set_examination_angles (&cc_subwindows [1], 0.f, 0.f);
	cc_subwindows[1].configuration = SHOW_BOTH;

	set_examination_angles (&cc_subwindows [2], 0.f, 90.f);
	cc_subwindows[2].configuration = SHOW_MAXILLA;

	set_examination_angles (&cc_subwindows [3], 180.f, 0.f);
	cc_subwindows[3].configuration = SHOW_BOTH;
	//cc_subwindows[3].scale_factor = .00000001;

#if N_SUBWINDOWS == 6
	cc_subwindows[4].configuration = SHOW_BOTH;
	set_examination_angles (&cc_subwindows [4], 90.f, 0.f);

	cc_subwindows[5].configuration = SHOW_MANDIBLE;
	set_examination_angles (&cc_subwindows [5], 0.f, -90.f);
#endif
#endif
	for (int i = 0; i < N_SUBWINDOWS; i++) {
		cc_subwindows[i].need_recenter = true;
	}

	//--
	
	for (i = 0; i < N_SUBWINDOWS; i++) {
		int win = subwindows [i];
		glutSetWindow (win);
		glutShowWindow ();
	}

	glutPostWindowRedisplay (main_window);
	doing_multiview = true;
}

void
multiview_off ()
{
	int i;
	float ary [3];
	for (i=0; i < 3; i++)
		ary[i] = cc_subwindows[0].viewpoints[i];
	widget_translation->set_float_array_val (ary);

	zoom_live_var = cc_subwindows[0].scale_factor;

	cc_main.configuration = SHOW_BOTH;
	cc_main.need_recenter = false;
	
	// Just reset these back to something known.
	for (i = 0; i < N_SUBWINDOWS; i++) {
		cc_subwindows[i].configuration = SHOW_BOTH;
		cc_subwindows[i].need_recenter = false;
	}
//--
		
	for (i = 0; i < N_SUBWINDOWS; i++) {
		int win = subwindows [i];
		glutSetWindow (win);
		glutHideWindow ();
	}

	glutPostWindowRedisplay (main_window);

	doing_multiview = false;
}

//---------------------------------------------------------------------------
// Name:	multiview_callback
// Purpose:	Callback function to handle checkbox activity.
//---------------------------------------------------------------------------
void 
multiview_callback (const int control)
{
	if (!model) 
		return;
	if (!widget_multiview)
		return;

	if (doing_cut_away || doing_manual_spacing || showing_occlusal) {
		widget_multiview->set_int_val (false);
		return;
	}

	doing_multiview = widget_multiview->get_int_val () ? true : false;

	// Reset the maxilla/mandible checkboxes.
	widget_maxilla->set_int_val (1);
	widget_mandible->set_int_val (1);
	if (doing_multiview) {
		widget_mandible->disable ();
		widget_maxilla->disable ();
		widget_cutaway->disable ();
		widget_manual_spacing->disable ();
	} else {
		widget_mandible->enable ();
		widget_maxilla->enable ();
		widget_cutaway->enable ();
		widget_manual_spacing->enable ();
	}
	
	if (doing_multiview) {
		multiview_on ();
	} else {
		multiview_off ();
	}
}

//---------------------------------------------------------------------------
// Name:	init_cross_section
// Purpose:	Initializes cross-section mode.
//---------------------------------------------------------------------------
void
init_cross_section (void)
{
	IndexedFaceSet *ifs1;
	IndexedFaceSet *ifs2;
	find_maxilla_mandible (ifs1, ifs2);
	if (ifs1) {
		ifs1->force_green = true;
		ifs1->doing_cross_section = true;
	}
	if (ifs2) {
		ifs2->force_green = false;
		ifs2->doing_cross_section = true;
	}

	if (which_cross_section)
		mouse_x = usable_width / 2;
	light0.reset ('c');
}
//---------------------------------------------------------------------------
// Name:	stop_cross_section
// Purpose:	Ends cross-section mode.
//---------------------------------------------------------------------------
void
stop_cross_section (void)
{
	IndexedFaceSet *ifs1;
	IndexedFaceSet *ifs2;
	find_maxilla_mandible (ifs1, ifs2);
	if (ifs1) {
		ifs1->force_green = false;
		ifs1->doing_cross_section = false;
	}
	if (ifs2) {
		ifs2->force_green = false;
		ifs2->doing_cross_section = false;
	}
}

//---------------------------------------------------------------------------
// Name:	distance_3d
// Purpose:	Calculates distance between 2 points.
//---------------------------------------------------------------------------
double
distance_3d (double cx, double cy, double cz, double cx2, double cy2, double cz2)
{
	double dx = cx - cx2;
	double dy = cy - cy2;
	double dz = cz - cz2;
	dx *= dx;
	dy *= dy;
	dz *= dz;
	return sqrt (dx+dy+dz);
}

//---------------------------------------------------------------------------
// Name:	gui_set_status
// Purpose:	Sets Maxilla-specific window title.
//---------------------------------------------------------------------------
void
gui_set_status (char *str) 
{
	ASSERT_NONZERO (str,"string")

	widget_status_line->set_text (str);
}

//---------------------------------------------------------------------------
// Name:	gui_set_window_title
// Purpose:	Sets Maxilla-specific window title.
//---------------------------------------------------------------------------
void
gui_set_window_title (char *title, char *filename) 
{
	char window_title[250];
	memset(window_title, 0, 250);
	if (title)
	{
		sprintf (window_title, 
		 "%s (%s) - Maxilla %s", title, filename ? filename : "no file", PROGRAM_RELEASE);
	} 
	else if (filename && !title)
	{	sprintf (window_title, 
		 "No title (%s) - Maxilla %s", filename, PROGRAM_RELEASE);
	}
	else		
	{	
		sprintf (window_title, "No file - Maxilla %s", PROGRAM_RELEASE);
	}

	if (!showing_bolton) {
		glutSetWindow (main_window);
	} else {
		glutSetWindow (secondary_window);
	}
	glutSetWindowTitle (window_title);
}

//---------------------------------------------------------------------------
// Name:	gui_set_title
// Purpose:	Sets Maxilla-specific window title.
//---------------------------------------------------------------------------
void
gui_set_title () 
{
#ifndef ORTHOCAST
	char tmp[250];
	if (model && model->title)
		sprintf (tmp, "Title: %s", model->title);
	else if (model && !model->title)
		sprintf (tmp, "Title: none");
	else
		sprintf (tmp, "Title: -");
	widget_title->set_text (tmp);
#endif
}

//---------------------------------------------------------------------------
// Name:	read_user_parameter32
// Purpose:	Reads a 32-bit user parameter.
//---------------------------------------------------------------------------
bool
read_user_parameter32 (const char *name, unsigned long &value_return)
{
	ASSERT_NONZERO (name,"name")

	char *homedir = NULL;
	char fullpath [PATH_MAX];

	value_return = 0;

#ifdef WIN32
	homedir = getenv ("HOMEPATH");
	strcpy (fullpath, "c:");
	strcat (fullpath, homedir);
	strcat (fullpath, "\\maxilla.cfg");
#else
	homedir = getenv ("HOME");
	strcpy (fullpath, homedir ? homedir : "/tmp");
	strcat (fullpath, "/.maxillarc");
#endif

	FILE *f = fopen (fullpath, "rb");
	if (!f)
		return false;

	while (true) {
		char key[100];
		unsigned long value;

		if (2 != fscanf (f, "%s 0x%08lx\n", key, &value))
			break;

		value &= 0xffffffff;

		if (!strcmp (key, name)) {
			value_return = value;
			fclose (f);
			return true;
		}
	}

	fclose (f);
	return false;
}

//---------------------------------------------------------------------------
// Name:	write_user_parameter32
// Purpose:	Writes a 32-bit user parameter.
//---------------------------------------------------------------------------
bool
write_user_parameter32 (const char *name, const unsigned long value)
{
	char *homedir = NULL;
	char fullpath [PATH_MAX];

#ifdef WIN32
	homedir = getenv ("HOMEPATH");
	strcpy (fullpath, "c:");
	strcat (fullpath, homedir);
	strcat (fullpath, "\\maxilla.cfg");
#else
	homedir = getenv ("HOME");
	strcpy (fullpath, homedir ? homedir : "/tmp");
	strcat (fullpath, "/.maxillarc");
#endif

#define MAX_RCFILE_DATA (100)
	char *keys [MAX_RCFILE_DATA];
	unsigned long values [MAX_RCFILE_DATA];
	int n_data = 0;

	FILE *f = fopen (fullpath, "rb");
	if (f) {
		while (true) {
			char key[100];
			unsigned long value2;

			if (2 != fscanf (f, "%s 0x%08lx\n", key, &value2))
				break;

			value2 &= 0xffffffff;

#ifdef WIN32
			keys [n_data] = _strdup (key);
#else
			keys [n_data] = strdup (key);
#endif
			values [n_data] = value2;
			n_data++;
			if (n_data >= MAX_RCFILE_DATA)
				break;
		}
		fclose (f);
	}

	// Search for existing key-value pair.
	int i = 0;
	bool found = false;
	while (i < n_data) {
		if (!strcmp (name, keys[i])) {
			values [i] = value;
			found = true;
			break;
		}
		i++;
	}
	if (!found) {
#ifdef WIN32
		keys [i] = _strdup (name);
#else
		keys [i] = strdup (name);
#endif
		values [i] = value;
		n_data++;
	}

	// Write the file out.
	f = fopen (fullpath, "wb");
	if (!f) 
		return false;

	for (i = 0; i < n_data; i++) {
		fprintf (f, "%s 0x%08lx\n", keys[i], 
			0xffffffff & values[i]);
		free (keys[i]);
	}
	fclose (f);

	return true;
}

//---------------------------------------------------------------------------
// Name:	retrieve_doctor_name
// Purpose:	Reads a 32-bit user parameter.
//---------------------------------------------------------------------------
bool
retrieve_doctor_name ()
{
	char *homedir = NULL;
	char fullpath [PATH_MAX];

#ifdef WIN32
	homedir = getenv ("HOMEPATH");
	strcpy (fullpath, "c:");
	strcat (fullpath, homedir);
	strcat (fullpath, "\\maxilla.cf2");
#else
	homedir = getenv ("HOME");
	strcpy (fullpath, homedir ? homedir : "/tmp");
	strcat (fullpath, "/.maxillarc2");
#endif

	FILE *f = fopen (fullpath, "rb");
	if (!f)
		return false;

	int len = fread (doctor_name, 1, PATH_MAX, f);

	if (len >= 0 && len <= PATH_MAX)
		doctor_name [len] = 0;
	else
		doctor_name [0] = 0;

	char *s = strchr (doctor_name, '\n');
	if (s) 
		*s = 0;
	s = strchr (doctor_name, '\r');
	if (s) 
		*s = 0;

	fclose (f);
	return len > 0;
}

//---------------------------------------------------------------------------
// Name:	store_doctor_name
// Purpose:	Stores the doctor name to a file.
//---------------------------------------------------------------------------
bool
store_doctor_name ()
{
	char *homedir = NULL;
	char fullpath [PATH_MAX];

#ifdef WIN32
	homedir = getenv ("HOMEPATH");
	strcpy (fullpath, "c:");
	strcat (fullpath, homedir);
	strcat (fullpath, "\\maxilla.cf2");
#else
	homedir = getenv ("HOME");
	strcpy (fullpath, homedir ? homedir : "/tmp");
	strcat (fullpath, "/.maxillarc2");
#endif

	FILE *f = fopen (fullpath, "wb");
	if (!f)
		return false;

	fwrite (doctor_name, 1, strlen (doctor_name), f);
	fprintf (f, "\n\n");
	fclose (f);

	return true;
}

long millisecond_time ()
{
#ifdef WIN32
	return timeGetTime();
#else
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return tv.tv_sec *1000 + tv.tv_usec/1000;
#endif
}

//---------------------------------------------------------------------------
// Name:	diag_write
// Purpose:	Appends a diagnostic string to the diagnostics log file.
// Note:	Caller need not put newline at the end.
//---------------------------------------------------------------------------
bool
diag_write (char *str)
{
#ifndef DISABLE_DIAGNOSTICS
	char *homedir = NULL;
	char fullpath [PATH_MAX];

#ifdef WIN32
	homedir = getenv ("HOMEPATH");
	strcpy (fullpath, "c:");
	strcat (fullpath, homedir);
	strcat (fullpath, "\\maxilla_diagnostics.txt");
#else
	homedir = getenv ("HOME");
	strcpy (fullpath, homedir ? homedir : "/tmp");
	strcat (fullpath, "/.maxilla_diagnostics.txt");
#endif

	FILE *f = fopen (fullpath, "ab");
	if (!f)
		return false;

	unsigned long t = millisecond_time () - time_program_start;
	unsigned long seconds = t/1000;
	unsigned long ms = t % 1000;

	fprintf (f, "%lu.%03lu: ", seconds, ms);
	fwrite (str, 1, strlen (str), f);
	fprintf (f, "\n");
	fclose (f);
#endif
	return true;
}

//---------------------------------------------------------------------------
// Name:	diag_write_int_int
// Purpose:	Appends a diagnostic string w/ two integers.
// Note:	Caller need not put newline at the end of the string.
//---------------------------------------------------------------------------
bool
diag_write_int_int (char *str, int i1, int i2)
{
	char tmp [1000];
	sprintf (tmp, "%s %d %d", str, i1, i2);
	return diag_write (tmp);
}

//---------------------------------------------------------------------------
// Name:	diag_init
// Purpose:	Clears the diagnostic log file.
//---------------------------------------------------------------------------
bool
diag_init ()
{
#ifndef DISABLE_DIAGNOSTICS
	char *homedir = NULL;
	char fullpath [PATH_MAX];

#ifdef WIN32
	homedir = getenv ("HOMEPATH");
	strcpy (fullpath, "c:");
	strcat (fullpath, homedir);
	strcat (fullpath, "\\maxilla_diagnostics.txt");
#else
	homedir = getenv ("HOME");
	strcpy (fullpath, homedir ? homedir : "/tmp");
	strcat (fullpath, "/.maxilla_diagnostics.txt");
#endif

#ifdef WIN32
	_unlink (fullpath);
#else
	unlink (fullpath);
#endif

	return diag_write ("Program start");
#else
	return true;
#endif
}

//---------------------------------------------------------------------------
// Name:	floats_to_rgb
// Purpose:	Converts floats to RGB.
// Note:	Each param is assumed to be in the range [-1,1].
//---------------------------------------------------------------------------
RGB
floats_to_rgb (const float *ary)
{
	RGB r = 255 & (int) (255.0f * ary[0]);
	RGB g = 255 & (int) (255.0f * ary[1]);
	RGB b = 255 & (int) (255.0f * ary[2]);
	g <<= 8;
	b <<= 16;
	return r|g|b;
}

//---------------------------------------------------------------------------
// Name:	floats_from_rgb
// Purpose:	Converts floats to RGB.
//---------------------------------------------------------------------------
void
floats_from_rgb (RGB rgb, float *ary)
{
	float r = (float) (255 & (rgb & 255)) / 255.0f;
	float g = (float) (255 & ((rgb >> 8) & 255)) / 255.0f;
	float b = (float) (255 & ((rgb >> 16) & 255)) / 255.0f;
	ary[0] = r;
	ary[1] = g;
	ary[2] = b;
}

//---------------------------------------------------------------------------
// Name:	save_user_settings
// Purpose:	Stores any important user-selectable settings.
//---------------------------------------------------------------------------
void 
save_user_settings ()
{
	RGB fg, bg;
	fg = floats_to_rgb (user_fg);
	bg = floats_to_rgb (user_bg);
	write_user_parameter32 ("foreground", fg);
	write_user_parameter32 ("background", bg);
}


//---------------------------------------------------------------------------
// Name:	xy_to_yaw_pitch
// Purpose:	Given an (x,y) 2D point, meant to indicate a point on the
//		2D rendering of a 3D a ball, determine what the vector is 
//		to the surface location that the (x,y) points to, and return
//		yaw & pitch (Euler rotation) to convey that.
//---------------------------------------------------------------------------
void
xy_to_yaw_pitch (const int x, const int y, 
				 const int max_x, const int max_y, 
				 float &yaw, float &pitch) 
{
	// Top view of ball:
	// <----- screen x ----->
	//           .          ^
	//         ^^ ^^        |
	//       ^^    /^^      | screen z
	//      ^     /   ^     |
	//      ^    /a   ^     |
	//      -----------     v
	// Angle a is sought, it's -pi/2 to +pi/2 radians.

	int m2 = max_x/2;
	float a;
	float tmp1 = (float) x;
	float tmp2 = (float) m2;
	float tmp3 = (tmp2-tmp1)/tmp2;
	if (tmp3 < -1.f)
		tmp3 = -1.f;
	if (tmp3 > 1.f)
		tmp3 = 1.f;
	a = asinf (tmp3);

	// Now do the same for y.
	int m3 = max_y/2;
	float b;
	tmp1 = (float) y;
	tmp2 = (float) m3;
	tmp3 = (tmp2-tmp1)/tmp2;
	if (tmp3 < -1.f)
		tmp3 = -1.f;
	if (tmp3 > 1.f)
		tmp3 = 1.f;
	b = asinf (tmp3);

	// All done.
	yaw = a;
	pitch = b;
}

//---------------------------------------------------------------------------
// Name:	rotate_about_x_axis
// Purpose:	Rotates one point about the X axis.
//---------------------------------------------------------------------------
void
rotate_about_x_axis (const double radians, double p[3]) 
{
	double c = cos (radians);
	double s = sin (radians);
	double y2 = p[1] * c - p[2] * s;
	double z2 = p[1] * s + p[2] * c;
	p[1] = y2;
	p[2] = z2;
}

//---------------------------------------------------------------------------
// Name:	rotate_about_y_axis
// Purpose:	Rotates one point about the Y axis.
//---------------------------------------------------------------------------
void
rotate_about_y_axis (const double radians, double p[3]) 
{
	double c = cos (radians);
	double s = sin (radians);
	double x2 = p[0] * c + p[2] * s;
	double z2 = p[2] * c - p[0] * s;
	p[0] = x2;
	p[2] = z2;
}

//---------------------------------------------------------------------------
// Name:	rotate_about_z_axis
// Purpose:	Rotates one point about the Z axis.
//---------------------------------------------------------------------------
void
rotate_about_z_axis (const double radians, double p[3]) 
{
	double c = cos (radians);
	double s = sin (radians);
	double x2 = p[0] * c - p[1] * s;
	double y2 = p[0] * s + p[1] * c;
	p[0] = x2;
	p[1] = y2;
}

////////////////////////////////////////////////////////////////////////////////
class OpenGLRenderContext: public CRenderContext
{
public:

	virtual void PushMatrix() {
		glPushMatrix();
	};
	virtual void PopMatrix() {
		glPopMatrix();
	}

	virtual void Translatef(float x, float y, float z) {
		glTranslatef(x,y,z);
	}
	virtual void Rotatef(float angle, float x, float y, float z) {
		glRotatef(angle, x,y,z);
	}
	virtual void Scalef(float x, float y, float z) {
		glScalef(x,y,z);
	}

	virtual void Triangle( Point* normal, Point* p1, Point* p2, Point* p3)
	{
		glNormal3f (normal->x, normal->y, normal->z);
		Point_express (p1);
		Point_express (p2);
		Point_express (p3);		
	};

	virtual void TriangleSmooth( Point* p1, Point* p2, Point* p3)
	{
		Point_express_smooth (p1);
		Point_express_smooth (p2);
		Point_express_smooth (p3);
	};

};

//---------------------------------------------------------------------------
// Name:	Triangle::Triangle
// Purpose:	Create a Triangle object and compute normal vector.
// Note:	Windows OpenGL does not seem to require a true normal
//			vector i.e. unit 1 length.
//---------------------------------------------------------------------------
Triangle::Triangle (Point *p1_, Point *p2_, Point *p3_) 
{
	double d1x, d1y, d1z, d2x, d2y, d2z;
	double cross_x, cross_y, cross_z;

	ASSERT_NONZERO (p1_,"point")
	ASSERT_NONZERO (p2_,"point")
	ASSERT_NONZERO (p3_,"point")
	//----------

	p1 = p1_;
	p2 = p2_;
	p3 = p3_;

	// Compute the normal vector.
	d1x = p2->x - p1->x;
	d1y = p2->y - p1->y;
	d1z = p2->z - p1->z;
	d2x = p3->x - p2->x;
	d2y = p3->y - p2->y;
	d2z = p3->z - p2->z;
	cross_x = d2y*d1z - d2z*d1y;
	cross_y = d2z*d1x - d2x*d1z;
	cross_z = d2x*d1y - d2y*d1x;

	float mag = 
		sqrt (cross_x*cross_x + cross_y*cross_y + cross_z*cross_z);

	normal_vector = Point_new (cross_x / mag, cross_y / mag, cross_z / mag);

	area = Point_distance (p1, p2) * Point_distance (p1, p3) / 2.0f;

	total_allocated += sizeof (Triangle);
}
///////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------
// Name:	Triangle::express
// Purpose:	Tell OpenGL about this Triangle.
//---------------------------------------------------------------------------
void Triangle::express (CRenderContext* pContext) 
{
	glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	if (p1 && p2 && p3 && normal_vector) {
		if (doing_smooth_shading 
			&& p1->valid_vertex_normal 
		    && p2->valid_vertex_normal 
			&& p3->valid_vertex_normal
			&& !p1->along_crease 
			&& !p2->along_crease
			&& !p3->along_crease) 
		{
			// Per-vertex normals are stored in Point objects.
			pContext->TriangleSmooth( p1, p2, p3 );
			//Point_express_smooth (pContext, p1);
			//Point_express_smooth (pContext, p2);
			//Point_express_smooth (pContext, p3);
		} else {
// STL executes thru here
			// Use the triangle's normal.
			
			pContext->Triangle( normal_vector, p1, p2, p3 );
			//glNormal3f (normal_vector->x, normal_vector->y, normal_vector->z);
			//Point_express (pContext, p1);
			//Point_express (pContext, p2);
			//Point_express (pContext, p3);
		}
	}
}

//---------------------------------------------------------------------------
// Name:	Triangle::express_as_lines
// Purpose:	Tell OpenGL about this Triangle.
//---------------------------------------------------------------------------
void Triangle::express_as_lines (CRenderContext* pContext) 
{
	glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	if (p1 && p2 && p3 && normal_vector) {
		// Use the triangle's normal.
		//
		glBegin (GL_LINES);
		Point_express (p1);
		Point_express (p2);
		Point_express (p3);
		glEnd ();
	}
}

//---------------------------------------------------------------------------
// Name:	InputWord
// Purpose:	Create InputWord from string.
//---------------------------------------------------------------------------
InputWord::InputWord (const char* s) 
{
	ASSERT_NONZERO (s,"string")
	//----------

#ifdef WIN32
	str = _strdup (s);
#else
	str = strdup (s);
#endif
	if (!str)
		warning ("Function strdup () returned NULL.");
	next = NULL;
	children = NULL;
	value = 0.f;

	total_allocated += strlen (s) + 1;
}


//---------------------------------------------------------------------------
// Name:	rotate_bounds_about_one_axis
// Purpose:	Rotates x/y/z bounds about the X axis.
//---------------------------------------------------------------------------
void 
rotate_bounds_about_one_axis (const double angle, 
	char which,
	double& xmin, double& xmax, double& ymin,
	double& ymax, double& zmin, double &zmax)
{
	int i;
	double p[8][3] = {
		{ xmin, ymin, zmin },  // Set up coordinates for all 8 corners.
		{ xmin, ymax, zmin },
		{ xmin, ymin, zmax },
		{ xmin, ymax, zmax },
		{ xmax, ymin, zmin },
		{ xmax, ymax, zmin },
		{ xmax, ymin, zmax },
		{ xmax, ymax, zmax }
	};

	// Rotate all 8 corners
	for (i=0; i<8; i++) {
		switch (which) {
		case 'x':	
			rotate_about_x_axis (angle, p[i]);
			break;
		case 'y':	
			rotate_about_y_axis (angle, p[i]);
			break;
		case 'z':	
			rotate_about_z_axis (angle, p[i]);
			break;
		}
	}

	// Reassess the maximum and minimum values.
	// Ideally we would retain all 8 points but for this program,
	// maintaining 8 points (24 floats) for each object is excessive.
	// So we just take that max/mins (6 floats).
	//
	xmin = ymin = zmin = 1E6f;
	xmax = ymax = zmax = -1E6f;
	for (i=0; i<8; i++) {
		if (p[i][0] < xmin)
			xmin = p[i][0];
		if (p[i][0] > xmax)
			xmax = p[i][0];

		if (p[i][1] < ymin)
			ymin = p[i][1];
		if (p[i][1] > ymax)
			ymax = p[i][1];

		if (p[i][2] < zmin)
			zmin = p[i][2];
		if (p[i][2] > zmax)
			zmax = p[i][2];
	}
}

//---------------------------------------------------------------------------
// Name:	get_model_maximum_dimension
// Purpose:	Determines the maximum of a model's width, height & depth.
//---------------------------------------------------------------------------
float 
get_model_maximum_dimension () 
{
	float model_width;
	float model_height;
	float model_depth;
	float maximum = 0.f;
	if (model) {
		model_width = model->maxx - model->minx;
		model_height = model->maxy - model->miny;
		model_depth = model->maxz - model->minz;
#ifdef DEBUG
		printf ("Model width = %g, height = %g, depth = %g,\n", model_width, model_height, model_depth);
#endif
		maximum = model_width > model_height ? model_width : model_height;
		maximum = maximum > model_depth ? maximum : model_depth;
	}
	return maximum;
}

//---------------------------------------------------------------------------
// Name:	auto_set_zoom
// Purpose:	Sets the zoom based on the model size.
//---------------------------------------------------------------------------
void
auto_set_zoom (CameraCharacteristics *cc)
{
	float max_dim = get_model_maximum_dimension ();

	// We're using a 1-degree field of view in orthographic mode.
	// tan (.5) = opposite / adjacent.
	float t = tanf (0.5f * 3.1415926536f / 180.f);

	// We're placing the object at z = -1.2.
	t *= 1.2f;

	// Only half of the height is above the axis.
	float max_allowed_dim = t * 2.f;

#ifdef ORTHOCAST
	cc->scale_factor = DEFAULT_ORTHOGRAPHIC_SCALE_FACTOR;
#else
	scale_factor = max_allowed_dim / max_dim;
	printf ("Auto zoomed to %g.\n", scale_factor);
#endif
	
	int i;
	float ary [3];
	for (i=0; i < 3; i++)
		ary[i] = cc->viewpoints[i];
	widget_translation->set_float_array_val (ary);

	zoom_live_var = cc->scale_factor;
	widget_zoom->set_z (zoom_live_var);

	for (int i=0; i < 3; i++)
		pan_live_vars [i] = cc->viewpoints [i];
	
	widget_translation->set_x (pan_live_vars[0]);
	widget_translation->set_y (pan_live_vars[1]);
	widget_translation->set_z (pan_live_vars[2]);
}


//---------------------------------------------------------------------------
// Name:	gui_reset
// Purpose:	Resets perspective, angles, scale.
//---------------------------------------------------------------------------
void
gui_reset (const bool reset_options)
{
	CameraCharacteristics *cc = &cc_main;

	if (reset_options) 
		doing_orthographic = true;

	doing_multiview = false;
	if (widget_multiview) {
		widget_multiview->enable ();
		widget_multiview->set_int_val (false);
	}

	doing_cut_away = false;
	widget_cutaway->enable ();
	widget_cutaway->set_int_val (false);
	
	widget_mandible->enable ();
	widget_maxilla->enable ();
	widget_maxilla->set_int_val (1);
	widget_mandible->set_int_val (1);

	widget_manual_spacing->enable ();
	widget_manual_spacing->set_int_val(false);
	doing_manual_spacing = false;

	showing_occlusal = false;

	cc->rotation_pitch = 0.f;
	cc->rotation_yaw = 0.f;

	cc->scale_factor = doing_orthographic ? DEFAULT_ORTHOGRAPHIC_SCALE_FACTOR 
		: DEFAULT_SCALE_FACTOR;

	auto_set_zoom (cc);
}


//---------------------------------------------------------------------------
// Name:	InputWord::~InputWord
// Purpose:	Destroys input word.
//---------------------------------------------------------------------------
InputWord::~InputWord ()
{
	if (str && str[0]!='#') // If word was not strdup'd, it starts w/ #.
		free (str);
	if (children)
		delete children;
	if (next)
		delete next;
}

//---------------------------------------------------------------------------
// Name:	getword
// Purpose:	Reads a text word from a VRML file.
// Returns:	Length or EOF.
//---------------------------------------------------------------------------
int
getword (InputFile *file, char *buf, const int len, bool *is_number_return)
{
	int ix=0, ch;
	bool in_str = false;
	bool is_number = true;
	char commas_in_number = 0;

	ASSERT_NONZERO (file,"file")
	ASSERT_NONZERO (buf,"buffer")
	//----------

	if (is_number_return)
		*is_number_return = false;

	while (EOF != (ch = file->getchar ())) {

		// Whitespace
		if (ch==' ' || ch=='\t' || ch=='\r' || ch=='\n' || ch==0xff) {
			if (!ix && !in_str)
				continue;
			else if (!in_str) {
				buf[ix]=0;
				break;
			}
		}	
		// Comments
		if (!in_str && ch=='#') {
			while (EOF != (ch = file->getchar ())) {
				if (ch=='\n')
					break;
			}
			if (ix) {
				buf[ix]=0;
				break;
			}
			continue;
		}
		// Quoted strings
		if (ch=='"') {
			is_number = false;
			if (!ix && !in_str) {
				in_str = true;
				continue;
			} else {
				if (!in_str)
					file->unget (ch);
				buf[ix]=0;
				return ix;
			}
		}
		// Brackets & braces
		if (!in_str && (ch=='[' || ch==']' || ch=='{' || ch=='}')) {
			if (ix) {
				buf[ix] = 0;
				file->unget (ch);
				break;
			} else {
				*buf = ch;
				buf[1] = 0;
				return 1;
			} 
		}
		// Text
		buf[ix++] = ch;
		if (!isdigit (ch) && ch != '.' && ch!='-' && ch!=',')
			is_number = false;
		if (ch==',' && is_number)
			commas_in_number++;
		if (ix == (len-1)) {
			buf[ix]=0;
			return ix;
		}
	}

	// Too many commas within a number means it is not a number.
	if (commas_in_number > 1)
		is_number = false;

	// If a number has a comma but it is not the ending char,
	// it is not a number.
	if (commas_in_number == 1 && buf[ix-1]!=',')
		is_number = false;

	// If the string consists only of a comma, it's not a number.
	if (commas_in_number == 1 && ix == 1)
		is_number = false;

	if (is_number_return)
		*is_number_return = is_number;

	return ix ? ix : (ch==EOF? EOF : 0);
}

//---------------------------------------------------------------------------
// Name:	vrml_reader_core
// Purpose:	Reads a VRML file, creating a tree of nodes.
//---------------------------------------------------------------------------
InputWord *
vrml_reader_core (InputFile *file, InputWord *parent)
{
#define MAXWORDLEN 1024
	char buf[MAXWORDLEN];
	InputWord *first = NULL;
	InputWord *last = NULL;
	bool is_number;

	ASSERT_NONZERO (file,"file")
	//----------

	while (true) {
		int len = getword (file, buf, MAXWORDLEN-1, &is_number);
		InputWord *w = NULL;
		char ch = buf[0];

		if (len==EOF) {
			break;
		}

		if (ch == '[' || ch == '{') {
			w = new InputWord (buf);
			if (!first) 
				first = w;
			else
				last->next = w;
			last = w;
			w->children = vrml_reader_core (file, last);
		}
		else if (ch == ']' || ch == '}') {
			return first;
		}
		else {
			InputWord *w;
			if (is_number) 
				w = new InputWord ((float) atof (buf));
			else
				w = new InputWord (buf);
			if (!first) {
				first = w;
			} else {
				last->next = w;
			}
			last = w;
		}
	}

	return first;
}

//---------------------------------------------------------------------------
// Name:	vrml_reader
// Purpose:	Reads a VRML file, creating a tree of nodes.
//---------------------------------------------------------------------------
InputWord *
vrml_reader (InputFile *file)
{
	ASSERT_NONZERO (file,"file")
	//----------

	if (!file->path)
		return NULL;

	if (!file->open ())
		return NULL;

	InputWord *n=NULL;
	n = vrml_reader_core (file, NULL);
	file->tree = n;

	file->close ();

	return n;
}

#if 0
// The following was written to test the reader
// and has not been used since.

//---------------------------------------------------------------------------
// Name:	inputword_dump_core
// Purpose:	Diagnostic dumper for tree of InputWord.
//---------------------------------------------------------------------------
void
inputword_dump_core (FILE *f, InputWord *n, int level)
{
	ASSERT_NONZERO (f,"file")
	ASSERT_NONZERO (n,"node")
	//----------

	while (n) {	
		int i = level;
		while (i--)
			fprintf (f, "\t");
		fprintf (f, "%s\n", n->str);
		if (n->children)
			inputword_dump_core (f, n->children, level+1);
		n = n->next;
	}
}

//---------------------------------------------------------------------------
// Name:	inputword_dump
// Purpose:	Diagnostic dumper for tree of InputWord.
//---------------------------------------------------------------------------
void
inputword_dump (FILE *f, InputWord *n)
{
	ASSERT_NONZERO (f,"file")
	ASSERT_NONZERO (n,"node")
	//----------

	inputword_dump_core (f, n, 0);
}
#endif

//---------------------------------------------------------------------------
// Name:	find_node_by_type
// Purpose:	Locates a child node of a given type.
//---------------------------------------------------------------------------
Node*
find_node_by_type (Node *node, char *type)
{
	Node *n;

	ASSERT_NONZERO (node,"node")
	ASSERT_NONZERO (type,"type")
	//----------

	if (!strcmp (node->type, type))
		return node;

	if (node->children) {
		n = find_node_by_type (node->children, type);
		if (n)
			return n;
	}

	if (node->next) {
		n = find_node_by_type (node->next, type);
		if (n)
			return n;
	}

	// Not found.
	return NULL;
}

//---------------------------------------------------------------------------
// Name:	node_dump_core
// Purpose:	Diagnostic dumper for tree of Node.
//---------------------------------------------------------------------------
void
node_dump_core (FILE *f, Node *n, const int level)
{
	ASSERT_NONZERO (f,"file")
	ASSERT_NONZERO (n,"node")
	//----------

	while (n) {	
		int i = level;
		while (i--)
			fprintf (f, "\t");
		n->dump (f);
		if (n->children)
			node_dump_core (f, n->children, level+1);
		n = n->next;
	}
}

//---------------------------------------------------------------------------
// Name:	node_dump
// Purpose:	Diagnostic dumper for tree of Node.
//---------------------------------------------------------------------------
void
node_dump (FILE *f, Node *n)
{
	ASSERT_NONZERO (f,"file")
	ASSERT_NONZERO (n,"node")
	//----------

	fprintf (f, "Tree of nodes:\n");
	node_dump_core (f, n, 0);
}


//---------------------------------------------------------------------------
// Name:	smooth
// Purpose:	Performs smoothing.
//---------------------------------------------------------------------------
void 
Model::smooth ()
{
	nodes->children->smooth ();
	smoothed = true;
}

//---------------------------------------------------------------------------
// Name:	smooth
// Purpose:	Smooths nodes.
//---------------------------------------------------------------------------
void Node::smooth ()
{
	if (children)
		children->smooth ();
	if (next)
		next->smooth ();

	if (!strcmp ("IndexedFaceSet", type)) {
		IndexedFaceSet *ifs;
		ifs = (IndexedFaceSet*) this;
		ifs->smooth_faces ();
	}
}

//---------------------------------------------------------------------------
// Name:	report_bounds
// Purpose:	Determines the bounding box for the model.
//---------------------------------------------------------------------------
void
Model::report_bounds ()
{
	minx = miny = minz = 1E6f;
	maxx = maxy = maxz = -1E6f;

	if (inputfile && inputfile->is_dst_file && main_switch) {
		Switch *sw = (Switch*) main_switch;
		sw->report_bounds (0, false, minx, maxx, miny, maxy, minz, maxz);
	}
	else if (nodes && nodes->children) {
		// We have to skip the Transform that we
		// probably inserted earlier.
		//
		nodes->children->report_bounds (0, true, minx, maxx, 
			miny, maxy, minz, maxz);
	}
}


//---------------------------------------------------------------------------
// Name:	Model::express
// Purpose:	Model to OpenGL converter. It "expresses" the model in 
//			OpenGL terms.
//---------------------------------------------------------------------------
void
Model::express (CRenderContext* pContext)
{
	if (doing_smooth_shading && !smoothed)
		model->smooth ();

	pContext->PushMatrix ();
	pContext->Translatef (model->translate_x,
				model->translate_y,
				model->translate_z);

	if (inputfile && inputfile->is_dst_file) {
		
		if(show_occlusal2 && occlusal2_node)
		{
			if(occlusal2_node->children)
			{
				//express each child with no sibling flag
				Node* n = occlusal2_node->children;
				while(n != NULL)
				{
					n->express(false, pContext);
					n = n->next;
				}
			}
			else
			{
				occlusal2_node->express(false, pContext);
			}
		}
		else if(main_switch)
		{
			Switch *sw = (Switch*) main_switch;
			sw->express (false, pContext);
		}
		else {
			nodes->children->express(true, pContext);
		}

	} else {
		nodes->children->express(true, pContext);
	}

	pContext->PopMatrix ();
}


//---------------------------------------------------------------------------
// Name:	occlusal_space_callback
// Purpose:	Callback for the two occlusal space buttons.
//---------------------------------------------------------------------------
void
occlusal_space_callback (const int control)
{
#if 0
	switch (control)
	{
		case 0:
			/* Y Translation:
			   Use the first value of occlusal_viewpoints to adjust how close the models are to each other.
			   occlusal_space is initialized with a value of METERS_PER_INCH / 2.f, so we add that to the
			   value to avoid a sudden jump in relative position the first time the widget is clicked. 
			*/

			occlusal_space = (occlusal_viewpoints[0] + (METERS_PER_INCH / 2.f));
			if (occlusal_top) {
				occlusal_top->second_translate_y = (occlusal_space + top_depth)/2.f;
				occlusal_bottom->second_translate_y = (occlusal_space + bottom_depth) / -2.f;
			}	
			break;
		case 1:
			/* X Translation:
			   Use the passed value in occlusal_viewpoints[0] to adjust the positions of occlusal_top
			   and occlusal_bottom.  Assign the positive value to the top and the inverse value to the
			   bottom so that the halves move in different directions.
			*/

			occlusal_top->second_translate_x = (occlusal_viewpoints[0]);
			occlusal_bottom->second_translate_x = - (occlusal_viewpoints[0]);
			break;
		case 2:
			/* Z Translation:
			   Use the passed value in occlusal_viewpoints[0] to adjust the positions of occlusal_top
			   and occlusal_bottom.  Assign the positive value to the top and the inverse value to the
			   bottom so that the halves move in different directions.
			*/

			occlusal_top->second_translate_z = (occlusal_viewpoints[0]);
			occlusal_bottom->second_translate_z = - (occlusal_viewpoints[0]);
	}
			
	glutPostRedisplay ();
#endif
}


//---------------------------------------------------------------------------
// Name:	manual_space_callback
// Purpose:	Callback for manual space widgets.
//---------------------------------------------------------------------------
void
manual_space_callback (const int control)
{
	if (!manual_spacing_bottom)
		return;
	if (doing_cut_away || showing_occlusal || doing_multiview)
		return;

	did_manual_spacing = true;

	// Store inital positions
	if (!manual_spacing_viewpoints[3])
		manual_spacing_viewpoints[3] = manual_spacing_bottom->translate_x;
	
	if (!manual_spacing_viewpoints[4])
		manual_spacing_viewpoints[4] = manual_spacing_bottom->translate_y;
	
	if (!manual_spacing_viewpoints[5])
		manual_spacing_viewpoints[5] = manual_spacing_bottom->translate_z;

	// Update bottom position
	switch (control)
	{
	case 0:
		manual_spacing_bottom->translate_x = 
			manual_spacing_viewpoints[0] + 
			manual_spacing_viewpoints[3];
		break;
	case 1:
		manual_spacing_bottom->translate_y = 
			manual_spacing_viewpoints[1] + 
			manual_spacing_viewpoints[4];
		break;
	case 2:
		manual_spacing_bottom->translate_z = 
			manual_spacing_viewpoints[2] + 
			manual_spacing_viewpoints[5];
		break;
	case 3: {
		float last_manual_rotation[16] = {
			1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1
		};

		// calculate rotation vector from rotation matrix
		float * r = manual_spacing_rotation;
		float angle = acos( ( r[0] + r[5] + r[10] - 1) / 2 ) / 4;

		// constrain maximum angle of rotation
		if( fabs(angle) > M_PI / 8 )
		{
			// reset to last rotation
			widget_relative_rotation->set_float_array_val(last_manual_rotation);
		}
		else
		{
			// save last rotation
			widget_relative_rotation->get_float_array_val(last_manual_rotation);

			// set this rotation
			manual_spacing_bottom->rotate_x = ( r[9] - r[6] ) / ( 2 * sin(angle));
			manual_spacing_bottom->rotate_y = ( r[2] - r[8] ) / ( 2 * sin(angle));
			manual_spacing_bottom->rotate_z = ( r[4] - r[1] ) / ( 2 * sin(angle));
			manual_spacing_bottom->rotate_angle = angle;
		}

		break;
	 }
	}
			
	glutPostRedisplay ();
}

CameraCharacteristics *
get_pertinent_cc ()
{	
	if (!doing_multiview)
		return &cc_main;

	// Figure out which window we're dealing with
	// so that we can affect only its camera
	// characteristics.
	//
	int win = glutGetWindow ();
	CameraCharacteristics *cc;
	if (win == subwindows [0])
		cc = &cc_subwindows [0];
	else if (win == subwindows [1])
		cc = &cc_subwindows [1];
	else if (win == subwindows [2])
		cc = &cc_subwindows [2];
	else if (win == subwindows [3])
		cc = &cc_subwindows [3];
#if N_SUBWINDOWS==6
	else if (win == subwindows [4])
		cc = &cc_subwindows [4];
	else if (win == subwindows [5])
		cc = &cc_subwindows [5];
#endif
	else 
		cc = &cc_main;

	return cc;
}

unsigned long
determine_clicked_triangle (double &cx, double &cy, double &cz)
{
	printf ("Entered %s\n", __FUNCTION__);

	cx = cy = cz = 0.f;
	
	// Redraw the scene but for the special purpose of
	// finding out which triangles intersect a certain
	// x,y location in the OpenGL window.
	//
	redrawing_for_selection = true;
	memset (selection_buffer, 0, SELECTION_BUFFER_SIZE * sizeof (GLint));
	draw_scene ();
	redrawing_for_selection = false;

	// OpenGL provides a list of triangles that were in 
	// the area that we previously specified using 
	// the mouse x/y coordinates.
	//
	int i;
	GLuint *ptr = selection_buffer;
	unsigned int min = 0xffffffff;
	unsigned int nearest = 0;
// printf ("selection_buffer_count = %d\n", selection_buffer_count);
	for (i = 0; i < selection_buffer_count; i++) {
		unsigned long n_names = *ptr++;
		unsigned long min_depth = *ptr++;
		unsigned long max_depth = *ptr++;
		printf ("Number of names = %ld, min_depth=%lu, max_depth=%lu\n", n_names, min_depth, max_depth);
		unsigned long name = 0;
		while (n_names) {
			name = *ptr++;
			n_names--;
		}
#if 0
		printf ("--------------------------\n");
		printf ("Selection buffer item %d has name %lu\n", i, name);
#endif
		if (min_depth < min) {
			min = min_depth;
			nearest = name;
		}
	}

	// 32-bit OSes only:
	//
	if (nearest && nearest != 0xffffffff) {
		Triangle *nearest_triangle = (Triangle*) nearest;
		nearest_triangle->get_center (cx, cy, cz);
		// printf ("Selected triangle 0x%08lx is at: %g,%g,%g\n", (unsigned long)nearest, cx, cy, cz);
		return nearest; // Return the "name", which is based on the Triangle pointer.
	} else {
		return 0;
	}
}

//---------------------------------------------------------------------------
// Name:	handle_mouse
// Purpose:	Callback for mouse button clicks.
//---------------------------------------------------------------------------
void 
handle_mouse (const int button, const int state, int x, int y)
{
	CameraCharacteristics *cc = get_pertinent_cc ();

	GLUI_Master.get_viewport_area (&usable_x, &usable_y, &usable_width, &usable_height);
	x -= usable_x;
	y -= usable_y;

	//printf ("y = %d\n", y);

	//----------------------------------------
	// In the case of cross-section, the
	// x coordinate determine where the cross
	// section is.
	//
	if (which_cross_section) {
		mouse_x = x;
		glutPostRedisplay ();
		mouse_button = button;
		return;
	}

	//----------------------------------------
	// Handle left mouse button down not while
	// measuring.
	//
	if (!doing_measuring && mouse_button == -1 && 
		button == GLUT_LEFT_BUTTON && 
		state == GLUT_DOWN) 
	{
		// Beginning a click or drag.
		//
		mouse_button = button;
		mouse_doing_click = true;
		mouse_x = x;
		mouse_y = y;
		cc->combined_rotation = cc->starting_rotation;
		cc->rotation_yaw = 0.f;
		cc->rotation_pitch = 0.f;
		mouse_dragging = true;
		mouse_t0 = millisecond_time ();
//MessageBoxA(0,"non-measuring mouse down",0,0);
		return;
	}

	if (doing_measuring && state == GLUT_UP)
	{
		mouse_button = -1;
		mouse_doing_click = false;
		mouse_x = x;
		mouse_y = y;
		return;
	}

	//----------------------------------------
	// If the mouse was already pressed and
	// we're not measuring, then just set the
	// ending rotation.
	//
	if (!doing_measuring &&
	    state == GLUT_UP && mouse_button == GLUT_LEFT_BUTTON 
	    && button == GLUT_LEFT_BUTTON)
	{
		mouse_button = -1;
		mouse_doing_click = false;
		mouse_x = x;
		mouse_y = y;

#if 0
		int dx = x - mouse_x;
		int dy = y - mouse_y;
		int distance;
		dx *= dx;
		dy *= dy;
		distance = (int) sqrt ((double) (dx + dy));
#endif

		cc->starting_rotation = cc->combined_rotation;
		cc->combined_rotation.x = 0.f;
		cc->combined_rotation.y = 0.f;
		cc->combined_rotation.z = 0.f;
		cc->combined_rotation.w = 0.f;
		cc->rotation_pitch = 0.0f;
		cc->rotation_yaw = 0.0f;
		mouse_x = 0;
		mouse_y = 0;
		mouse_dragging = false;
		mouse_button = -1;

		redraw_all ();
		return;
	}

	//----------------------------------------
	// Handle the creation of the red dot
	// while measuring.
	//
	// In the rotation-not-locked situation,
	// this is done at mouse-up.
	//
	// In the rotation-locked situation,
	// this is done at mouse-down.
	//
	if (doing_measuring && 
		/* mouse_button == -1 */ 
		state == GLUT_DOWN
		)
	{
		//------------------------------
		// Placing of the red dot.
		//
		unsigned long nearest = 0;
		double cx, cy, cz;
		mouse_button = button;
		mouse_doing_click = false;
		mouse_x = x;
		mouse_y = y;

		if (0 != (nearest = determine_clicked_triangle (cx, cy, cz))) 
		{
			red_dot_info *r = new red_dot_info (cx, cy, cz, nearest);

			if (red_dot_stack) {
				//----------------------------------------
				// Get distances to report to user.
				//
				float dist = distance_3d (cx, cy, cz, 
					red_dot_stack->x, red_dot_stack->y, red_dot_stack->z) * 1000.f;
				float dist_x = fabs (cx - red_dot_stack->x) * 1000.f;
				float dist_y = fabs (cy - red_dot_stack->y) * 1000.f;
				float dist_z = fabs (cz - red_dot_stack->z) * 1000.f;

				//
				// If in main screen, display distance there.
				//
				if (!showing_bolton) {
					char tmp[300];
#ifndef ORTHOCAST
					sprintf (tmp, "XYZ distance = %g mm, X distance = %g mm, Y distance = %g mm, Z distance = %g mm",
						dist, dist_x, dist_y, dist_z);
#else
					int left, right;
					left = (int) (10.f * dist);
					right = left % 10;
					left /= 10;
					sprintf (tmp, "Distance = %d.%d mm", left, right);
#endif
					gui_set_status (tmp);
				}
				else {
					//--------------------
					// If in secondary screen, we report the distance
					// in the text field widgets.
					// Only allow 2 dots.
					//
					if (red_dot_stack->next) {
						delete red_dot_stack->next;
						red_dot_stack->next = NULL;
					}
					set_secondary_value (secondary_current_measurement, dist);
				}
			}

			// After reporting distance, store the red dot.
			//
			push_dot (r);

			redraw_all ();
		}

		return;
	}
}


//---------------------------------------------------------------------------
// Name:	handle_motion
// Purpose:	Callback for keypresses involving mouse motion.
//---------------------------------------------------------------------------
void 
handle_motion (int x, int y)
{
	GLUI_Master.get_viewport_area (&usable_x, &usable_y, &usable_width, &usable_height);
	x -= usable_x;
	y -= usable_y;
	int dx = x - mouse_x;
	int dy = y - mouse_y;

	int win = glutGetWindow ();
	CameraCharacteristics *cc = get_pertinent_cc ();
	cc->window = win;

	if (mouse_button == GLUT_LEFT_BUTTON && which_cross_section) {
		mouse_x = x;
		redraw_all ();
		return;
	}

	//----------------------------------------
	// While measuring, allow for moving the
	// current red dot around. Rotation must
	// be locked, of course.
	//
	if (rotation_locked && doing_measuring && 
	    red_dot_stack && mouse_button == GLUT_LEFT_BUTTON)
	{
		unsigned long nearest = 0;
		double cx, cy, cz;

		mouse_x = x;
		mouse_y = y;

		if (0 != (nearest = determine_clicked_triangle (cx, cy, cz)))
		{
			Triangle *t = (Triangle*) nearest;

			// printf ("New location %g %g %g\n", cx, cy, cz);
			red_dot_stack->x = cx;
			red_dot_stack->y = cy;
			red_dot_stack->z = cz;
			red_dot_stack->name = (GLuint) nearest;

			red_dot_info *d2 = red_dot_stack->next;
			if (d2) {
				float dist = 1000.f * distance_3d (cx, cy, cz, d2->x, d2->y, d2->z);
#if 0
				printf ("d2: %g,%g,%g\n", d2->x, d2->y, d2->z);
				printf ("current: %g,%g,%g\n", cx, cy, cz);
#endif
				if (!showing_bolton) {
					char tmp[300];
					int left, right;
					left = (int) (10.f * dist);
					right = left % 10;
					left /= 10;
					sprintf (tmp, "Distance = %d.%d mm", left, right);
//	puts (tmp);
					gui_set_status (tmp);
				} else {
//	printf ("DISTANCE %g\n", dist);
					set_secondary_value (secondary_current_measurement, dist);
				}
			}

			draw_scene ();
		}

		return;
	}

	if (mouse_button == GLUT_LEFT_BUTTON) {
		printf ("dx,dy=%d,%d\n", dx, dy);

		if (mouse_doing_click && (dx < -5 || dx > 5 || dy < -5 || dy > 5))
			mouse_doing_click = false;

		cc->rotation_yaw = -0.5f * (float)dx;
		cc->rotation_pitch = -0.5f * (float)dy;

		if (!doing_multiview)
			glutPostWindowRedisplay (showing_bolton ?
				secondary_window : main_window);
		else 
			glutPostWindowRedisplay (win);

	}
}

#ifdef WIN32
void*
about_thread (void *foo)
{
		widget_status_line->set_text ("Starting web browser to show help information.");
		HINSTANCE hinst = ShellExecuteA (NULL, "open", "http://orthocast.com/maxilla_help.html", NULL, NULL, SW_SHOWNORMAL);
		return NULL;
}
#endif

#ifdef WIN32
void*
download_thread (void *foo)
{
		widget_status_line->set_text ("Starting web browser..");
		HINSTANCE hinst = ShellExecuteA (NULL, "open", "http://orthocast.com/ws_og/", NULL, NULL, SW_SHOWNORMAL);
		return NULL;
}
#endif

void popup_downloadModels()
{
#if defined(ORTHOCAST) && defined(WIN32)	
		CreateThread (0,0, (LPTHREAD_START_ROUTINE) download_thread, 0,0,0);			
#endif
}

//---------------------------------------------------------------------------
// Name:	popup_about
// Purpose:	Displays the proverbial About box.
//---------------------------------------------------------------------------
void
popup_about (bool only_help)
{
#if defined(ORTHOCAST) && defined(WIN32)
	if (only_help) {
		CreateThread (0,0, (LPTHREAD_START_ROUTINE) about_thread, 0,0,0);
		return;
	}
#endif

	char *msg = 
#ifndef ORTHOCAST
"This is Ortho-graphics Maxilla viewer version "PROGRAM_VERSION"\r\n\
Copyright (C) 2008-2013 by Zack T Smith and Ortho Cast Inc.\r\n\
For more information go to http://zsmith.co\r\n\
This program is covered by the GNU Public License v2.\r\n\
It is provided AS-IS. Use it at your own risk.\r\n\
See the included file entitled COPYING for more details.";
#else
"This is Ortho-graphics Maxilla viewer version "PROGRAM_RELEASE"\r\n\
Copyright (C) 2008-2013 by Ortho Cast Inc. and Zack T Smith\r\n\
It is provided AS-IS. Use it at your own risk.\r\n\
For more information go to http://OrthoCast.com";
#endif

#ifdef WIN32
	MessageBoxA (NULL, msg, "About Maxilla", MB_ICONINFORMATION | MB_OK);
#endif

#ifdef __APPLE__
	macosx_message_box ("About Maxilla", msg);
#endif

#ifdef __linux__
	linux_message_box ("About Maxilla", msg);
#endif
}

//---------------------------------------------------------------------------
// Name:	popup_key_help
// Purpose:	Displays a popup window showing help information about keys.
//---------------------------------------------------------------------------
void
popup_key_help ()
{
	char *msg = "\
Important Keys:\r\n\
\r\n\
a = Information about the program.\r\n\
h = Display this help information.\r\n\
\r\n\
___File operations___\r\n\
o = Open file.\r\n\
w = Close file.\r\n\
\r\n\
___Object examination___\r\n\
Up arrow = Rotate up.\r\n\
Back arrow = Rotate down.\r\n\
Left arrow = Rotate leftward.\r\n\
Right arrow = Rotate rightward.\r\n\
PageUp = Zoom in.\r\n\
PageDown = Zoom out.\r\n\
Home = Reset zoom.\r\n\
i = Translate up.\r\n\
m = Translate down.\r\n\
j = Translate left.\r\n\
k = Translate right.\r\n\
\r\n\
___Other stuff___\r\n\
g = Toggle orthographic perspective.\r\n\
d = Write BMP image file.\r\n\
q = Exit the program.\r\n";

#ifdef WIN32
	MessageBoxA (NULL, msg, "Help", MB_ICONINFORMATION | MB_OK);
#endif

#ifdef __APPLE__
	macosx_message_box ("Help", msg);
#endif

#ifdef __linux__
	linux_message_box ("Help", msg);
#endif
}

void load_file (InputFile*);
void close_file ();

//-----------------------------------------------------------------------------
// Name:	save_file_thread
//-----------------------------------------------------------------------------

void*
save_file_thread (void *foo)
{
	if (popup_active)
		return NULL;

	op_path [0] = 0;

#ifdef __APPLE__
	macosx_file_save (op_path, PATH_MAX-1);
#endif

#ifdef WIN32
	LPTSTR ptn = (LPTSTR) foo;

	popup_active = true;

	// It's OK to call this because the main
	// GUI event loop is still running.
	//
	OPENFILENAME ofn;
	TCHAR szFile[260];
	ZeroMemory (&ofn, sizeof(ofn));
	szFile[0]=0;
	ofn.Flags = OFN_OVERWRITEPROMPT;
	ofn.hwndOwner = NULL; 
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = (LPTSTR) szFile;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrFilter = ptn? ptn : _T("All\0*.*\0Text\0*.txt\0");
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrInitialDir = _T("");
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = sizeof(szFile);
	ofn.nMaxFileTitle = 0;

	if (TRUE == GetSaveFileName(&ofn)) {
		int i;
		for (i = 0; szFile[i]; i++)
			op_path [i] = szFile [i];
		op_path[i] = 0;
	} else {
		popup_active = false;
		op_path [0] = 0;
		ops_init ();
		ops_pause = false;

		return NULL;
	}			

#endif

	popup_active = false;

	if (!ops_get())
		return NULL;

	if (ops_pause)
		ops_pause = false;
	return NULL;
}


void*
save_stlfile_thread (void *foo)
{
	if (popup_active)
		return NULL;

	op_path [0] = 0;

#ifdef __APPLE__
	macosx_file_save (op_path, PATH_MAX-1);
#endif

#ifdef WIN32
	LPTSTR ptn = (LPTSTR) foo;

	popup_active = true;

	// It's OK to call this because the main
	// GUI event loop is still running.
	//
	OPENFILENAME ofn;
	ZeroMemory (&ofn, sizeof(ofn));

	TCHAR szFile[MAX_PATH];
	ZeroMemory (szFile, sizeof(TCHAR)*MAX_PATH);
	strcpy(szFile, ptn);	
	
	ofn.Flags = OFN_OVERWRITEPROMPT;
	ofn.hwndOwner = NULL; 
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = (LPTSTR) szFile;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrFilter = _T("STL\0*.stl\0All\0*.*\0");
	ofn.lpstrInitialDir = NULL;
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = sizeof(szFile);
	ofn.nMaxFileTitle = 0;

	if (TRUE == GetSaveFileName(&ofn)) {
		int i;
		for (i = 0; szFile[i]; i++)
			op_path [i] = szFile [i];
		op_path[i] = 0;
	} else {
		popup_active = false;
		op_path [0] = 0;
		ops_init ();
		ops_pause = false;

		return NULL;
	}			

#endif

	popup_active = false;

	if (!ops_get())
		return NULL;

	if (ops_pause)
		ops_pause = false;
	return NULL;
}



#ifdef WIN32
#define strcasecmp stricmp
#endif
int
handle_stl (char *path)
{
	int len = strlen (path);
	if (len < 4) {
		return 0;
	} else {
		char *s;
		s = path + len - 3;
		if (strcasecmp ("stl", s)) {
			return 0;
		} else {
			FILE *f = fopen (path, "r");
			if (!f) {
#ifdef WIN32
				MessageBox (0, _T("Can't open file."), 0,0);
#else
				warning ("Cannot open file.");
#endif
				popup_active = false;
				return -1;
			} else {
				fclose (f);
				if (model) {
					delete model;
					model = NULL;
				}

				bool is_pair_of_files = false;
				bool is_pair_of_files_upper = false;
				bool is_pair_of_files_lower = false;
				bool is_maxillar = false;
				bool is_mandibular = false;

				s = path + len - strlen ("_Maxillar.stl");
				if (s < path) {
					// is_pair_of_files = false;
				} else {
					if (!stricmp (s, "_Maxillar.stl"))
						is_maxillar = true;
					else {
						s = path + len - strlen ("_Mandibular.stl");
						if (!stricmp (s, "_Mandibular.stl"))
							is_mandibular = true;
					}

					

					if (is_maxillar || is_mandibular)
						is_pair_of_files = true;
					else
					{

						//s = path + len - strlen (" Upper.stl");
						//if (!stricmp (s, " Upper.stl"))
						//	is_pair_of_files_upper = true;
						//if (!stricmp (s, " Lower.stl"))
						//	is_pair_of_files_lower = true;
					}
				}

				if ( (!is_pair_of_files_upper && !is_pair_of_files_lower) && ((!is_pair_of_files) || (!is_maxillar && !is_mandibular))) {
					model = stl_parser_one_file (path);
				} else {

					if(is_pair_of_files_upper || is_pair_of_files_lower)
					{
						s = path + len - 1;
						while (s >= path && *s != ' ')
							s--;
						if (s < path) {
							popup_active = false;
							return NULL; // XX 
						}
						*s = 0;
					}
					else 
					{
						s = path + len - 1;
						while (s >= path && *s != '_')
							s--;
						if (s < path) {
							popup_active = false;
							return NULL; // XX 
						}
						*s = 0;
					}

					char path1 [PATH_MAX];
					char path2 [PATH_MAX];
					strcpy (path1, path);
					strcpy (path2, path);

					if(is_pair_of_files_upper || is_pair_of_files_lower)
					{
						strcat (path1, " Lower.stl");
						strcat (path2, " Upper.stl");					
					}
					else
					{
						strcat (path1, "_Mandibular.stl");
						strcat (path2, "_Maxillar.stl");					
					}

					model = stl_parser_two_files (path1, path2);
				}
				return 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Name:	open_file_thread
//-----------------------------------------------------------------------------

void*
open_file_thread (void *foo)
{
	if (popup_active)
		return NULL;

	char path [PATH_MAX];

	popup_active = true;

#ifdef WIN32
	OPENFILENAME ofn;
	TCHAR szFile[260];
	szFile[0] = 0;
	ZeroMemory (&ofn, sizeof (ofn));
	ofn.lpstrInitialDir = _T ("");

	// Initialize OPENFILENAME
	ofn.lStructSize = sizeof (ofn);
	ofn.hwndOwner = NULL; // hwnd;
	ofn.lpstrFile = (LPTSTR) szFile;
	ofn.lpstrFile[0] = _T('\0');
	ofn.nMaxFile = sizeof (szFile);
	ofn.lpstrFilter = _T ("All\0*.*\0VRML\0*.wrl\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (TRUE == GetOpenFileName (&ofn)) {
		int i=0;
	
		glFlush ();

		while (i < (PATH_MAX-1) && szFile[i])
			path[i++] = (char) szFile[i];
		path[i]=0;
	}
	else {
		popup_active = false;
		return NULL; // User cancelled popup.
	}
#endif

	popup_active = false;

#ifdef __APPLE__
	if (!macosx_file_open (path, PATH_MAX)) {
		return NULL;
	} 
#endif

#ifdef __linux__
	if (!linux_file_open (path, PATH_MAX))
		return;
#endif

	//----------------------------------------
	// First attempt STL.
	// If no good, continue with VRML.
	//
	int rv = handle_stl (path);
	if (rv < 0) 
		return NULL;
	if (rv > 0) {
		puts ("Successfully loaded STL.");
	} else {
		printf ("Initiating OP_OPEN\n");

		// If we reach this point, we have a path
		//
		strcpy (op_path, path);
		ops_add (OP_OPEN, true);
		ops_pause = false;
	}

	glutPostRedisplay ();

	return NULL;
}


//---------------------------------------------------------------------------
// Name:	glui_open_file_callback
// Purpose:	Performs the file-open function, reachable via menu or keypress.
//---------------------------------------------------------------------------
void 
glui_open_file_callback (int foo)
{
	if (popup_active)
		return;

	op_path [0] = 0;

	if (doing_multiview)
		multiview_off ();

	ops_pause = true;
#ifndef __APPLE__
	CreateThread (0, 0,(LPTHREAD_START_ROUTINE) open_file_thread, 0, 0, 0);
#else
	open_file_thread (0);
#endif
}

//---------------------------------------------------------------------------
// Name:	menu_close
// Purpose:	Performs file-close function, reachable via menu or keypress.
//---------------------------------------------------------------------------
void 
menu_close ()
{
	CameraCharacteristics *cc = get_pertinent_cc ();

	if (input_file) {
		close_file ();
		if (model) {
			delete model;
			model = NULL;
		}
		
		gui_reset (false);
		glutPostRedisplay ();
	}
}

//---------------------------------------------------------------------------
// Name:	glui_zoom_callback
// Purpose:	Callback function to handle zoom widget.
//---------------------------------------------------------------------------
void 
glui_zoom_callback (const int control)
{
	if (!doing_multiview) {
		if (zoom_live_var < 0.00001)
			zoom_live_var = 0.00001;
		cc_main.scale_factor = zoom_live_var;
		glutPostWindowRedisplay (showing_bolton? secondary_window
				: main_window);
	}
	else {
		int i;
		for (i = 0; i < N_SUBWINDOWS; i++) {
			cc_subwindows[i].scale_factor = zoom_live_var;	
			glutPostWindowRedisplay (subwindows [i]);
		}
	}
}

//---------------------------------------------------------------------------
// Name:	glui_translate_callback
// Purpose:	Callback function to handle translate widget.
//---------------------------------------------------------------------------
void
glui_translate_callback (const int control)
{
	if (!doing_multiview) {
		cc_main.viewpoints [0] = pan_live_vars [0];
		cc_main.viewpoints [1] = pan_live_vars [1];
		cc_main.viewpoints [2] = pan_live_vars [2];
		
		glutPostWindowRedisplay (showing_bolton? secondary_window
				: main_window);
	} else {
		for (int i = 0; i < N_SUBWINDOWS; i++) {
			cc_subwindows[i].viewpoints [0] = pan_live_vars [0];
			cc_subwindows[i].viewpoints [1] = pan_live_vars [1];
			cc_subwindows[i].viewpoints [2] = pan_live_vars [2];
			glutPostWindowRedisplay (subwindows [i]);
		}
	}
}

//---------------------------------------------------------------------------
// Name:	handle_menu
// Purpose:	Callback for GLUT popup menu activity.
//---------------------------------------------------------------------------
void 
handle_menu (const int selection)
{
	switch (selection) {
	case 1:
		glui_open_file_callback (0);
		break;
	case 2:
		menu_close ();
		break;
	case 3:
		popup_key_help ();
		break;
	case 99:
		save_user_settings ();
		myexit (EXIT_SUCCESS);
	}
}

//---------------------------------------------------------------------------
// Name:	gui_update_perspective
// Purpose:	Updates the GLUI widget for perspective setting.
//---------------------------------------------------------------------------
void
gui_update_perspective ()
{
#ifndef ORTHOCAST
	widget_ortho->set_int_val (doing_orthographic ? 1 : 0);
#endif
}

//---------------------------------------------------------------------------
// Name:	handle_keypress
// Purpose:	Callback for keypresses involving ASCII keys.
//---------------------------------------------------------------------------
void 
handle_keypress (unsigned char key, const int x, const int y)
{
	CameraCharacteristics *cc = get_pertinent_cc ();

	key = tolower (key);

	switch (key) {
	case 'o':
		glui_open_file_callback (0);
		break;

	case 'g':
		doing_orthographic = !doing_orthographic;
		cc->scale_factor = doing_orthographic ? 
		  DEFAULT_ORTHOGRAPHIC_SCALE_FACTOR : DEFAULT_SCALE_FACTOR;
		light0.reset (doing_orthographic? 'o' : 'n');
		gui_update_perspective ();
		glutPostRedisplay ();
		break;

	case 'w': // Close file.
		menu_close ();
		break;

	case 'h':
		popup_key_help ();
		break;

	case 'a':
		popup_about (false);
		break;

	case 'd':
		dump_screen_to_bmp ();
		break;

	case 't':
		ops_add (OP_TEST, false);
		break;

	case 'p':
		glui_print_callback(0);
		break;

	case 'i':
		arbitrary_y_translate += 0.001f; // XX update for % of total dims
		glutPostRedisplay ();
		break;
	case 'm':
		arbitrary_y_translate -= 0.001f;
		glutPostRedisplay ();
		break;
	case 'j':
		arbitrary_x_translate -= 0.001f;
		glutPostRedisplay ();
		break;
	case 'k':
		arbitrary_x_translate += 0.001f;
		glutPostRedisplay ();
		break;

	case 'q':
	case 27:	
		save_user_settings ();
		myexit (EXIT_SUCCESS);

	// Keys for moving lights and changing brightnesses.
	//
#if 0
	case 'm':
		ortho_z_adjust += 0.1;
		glutPostRedisplay ();
		break;
	case 'n':
		printf ("%lf,%lf,%lf\n",
			light0.pos[0],light0.pos[1],light0.pos[2]);
		printf ("%lf,%lf,%lf\n",
			light0.ambient,light0.brightness,light0.specularity);
		break;
	case 'i':
		// move light0 in +z direction
		light0.pos[2] += 1.f;
		light0.dump (0);
		glutPostRedisplay ();
		break;
	case 'k':
		// move light0 in -z direction
		light0.pos[2] -= 1.f;
		light0.dump (0);
		glutPostRedisplay ();
		break;
	case 'u':
		// move light0 in +y direction
		light0.pos[1] += 1.f;
		light0.dump (0);
		glutPostRedisplay ();
		break;
	case 'j':
		// move light0 in -y direction
		light0.pos[1] -= 1.f;
		light0.dump (0);
		glutPostRedisplay ();
		break;
	case '1':
		light0.brightness += 0.1f;
		light0.dump (0);
		glutPostRedisplay ();
		break;
	case '2':
		light0.brightness -= 0.1f;
		light0.dump (0);
		glutPostRedisplay ();
		break;
	case '3':
		light0.ambient += 0.1f;
		light0.dump (0);
		glutPostRedisplay ();
		break;
	case '4':
		light0.ambient -= 0.1f;
		light0.dump (0);
		glutPostRedisplay ();
		break;
#endif
	}
}

//---------------------------------------------------------------------------
// Name:	handle_special
// Purpose:	Callback for special keyboard characters, e.g. PgUp.
//---------------------------------------------------------------------------
void 
handle_special (const int key, const int x, const int y)
{
	CameraCharacteristics *cc = get_pertinent_cc ();

	switch (key) {
	case GLUT_KEY_DOWN:
	case GLUT_KEY_UP:
	case GLUT_KEY_RIGHT:
	case GLUT_KEY_LEFT: {
		switch (key) {
		case GLUT_KEY_LEFT: 
			cc->rotation_yaw = 5.f; 
			cc->rotation_pitch = 0.f; 
			break;
		case GLUT_KEY_RIGHT: 
			cc->rotation_yaw = -5.f; 
			cc->rotation_pitch = 0.f; 
			break;
		case GLUT_KEY_UP: 
			cc->rotation_pitch = 5.f; 
			cc->rotation_yaw = 0.f; 
			break;
		case GLUT_KEY_DOWN: 
			cc->rotation_pitch = -5.f; 
			cc->rotation_yaw = 0.f; 
			break;
		}
		Quat q;
		q.from_euler (cc->rotation_yaw, cc->rotation_pitch, 0.f, true);

		cc->combined_rotation.multiply (& cc->starting_rotation, &q);
		cc->starting_rotation = cc->combined_rotation;

		glutPostRedisplay ();
		}
		break;
	case GLUT_KEY_PAGE_UP:
		cc->scale_factor *= 1.1f;
		glutPostRedisplay ();
		break;
	case GLUT_KEY_PAGE_DOWN:
		if (cc->scale_factor >= 0.00001f)
			cc->scale_factor /= 1.1f;
		glutPostRedisplay ();
		break;	
	case GLUT_KEY_HOME:
		gui_reset (false);
		glutPostRedisplay ();
		break;
	}
}

void
changesize (CameraCharacteristics *cc, int w, int h)
{
	subwindow_aspect = (float) w / (float) h;

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	
	glViewport (0, 0, w, h);
	
	gluPerspective (doing_orthographic? 1.f : (using_preset_fieldofview? 45.f : cc->field_of_view),  // angle
		   subwindow_aspect,
		   doing_orthographic ? 0.1f : 0.01f,  // near z clipping
		   30.f); // far z clipping

	glMatrixMode (GL_MODELVIEW);
}

//---------------------------------------------------------------------------
// Name:	handle_resize
// Purpose:	Callback for window resizes.
//---------------------------------------------------------------------------
void 
handle_resize (const int w, const int h) 
{
#if 0
	printf ("In handle_resize, usable_width=%d, usable_height=%d\n",
		usable_width, usable_height);
#endif

	if (showing_bolton) {
		glutSetWindow (secondary_window);
		GLUI_Master.get_viewport_area (&usable_x, &usable_y, &usable_width, &usable_height);

		glViewport (usable_x, usable_y, usable_width, usable_height);
		xy_aspect = (float)usable_width / (float)usable_height;
	}
	else {
		glutSetWindow (main_window);
		GLUI_Master.get_viewport_area (&usable_x, &usable_y, &usable_width, &usable_height);
		glViewport (usable_x, usable_y, usable_width, usable_height);
		xy_aspect = (float)usable_width / (float)usable_height;

		int interwindow_space = 4;
#if N_WINDOWS==4
		int w2 = (usable_width - interwindow_space)/2;
#else
		int w2 = (usable_width - 2*interwindow_space)/3;
#endif
		int subwin_extra_height = 10;
		usable_height += subwin_extra_height;
		int h2 = (usable_height - interwindow_space)/2;

		subwindows_width = w2;
		subwindows_height = h2;

		// Top left
		glutSetWindow (subwindows [0]);
		glutPositionWindow (usable_x, usable_y);
		glutReshapeWindow (w2, h2);
		changesize (&cc_subwindows [0], w2, h2);
		if (doing_multiview)
			draw_scene ();

		// Top middle
		glutSetWindow (subwindows [1]);
		glutPositionWindow (usable_x + w2 + interwindow_space, usable_y);
		glutReshapeWindow (w2, h2);
		changesize (&cc_subwindows [1], w2, h2);
		if (doing_multiview)
			draw_scene ();

		// Lower left
		glutSetWindow (subwindows [2]);
		glutPositionWindow (usable_x, usable_y + h2 + interwindow_space);
		glutReshapeWindow (w2, h2);
		changesize (&cc_subwindows [2], w2, h2);
		if (doing_multiview)
			draw_scene ();

		// Lower middle
		glutSetWindow (subwindows [3]);
		glutPositionWindow (usable_x + w2 + interwindow_space, usable_y + h2 + interwindow_space);
		glutReshapeWindow (w2, h2);
		changesize (&cc_subwindows [3], w2, h2);
		if (doing_multiview)
			draw_scene ();

		// Lower middle
		glutSetWindow (subwindows [3]);
		glutPositionWindow (usable_x + w2 + interwindow_space, usable_y + h2 + interwindow_space);
		glutReshapeWindow (w2, h2);
		changesize (&cc_subwindows [3], w2, h2);
		if (doing_multiview)
			draw_scene ();
		
#if N_SUBWINDOWS==6
		// Top right
		glutSetWindow (subwindows [4]);
		glutPositionWindow (usable_x + 2*w2 + 2*interwindow_space, usable_y);
		glutReshapeWindow (w2, h2);
		changesize (&cc_subwindows [4], w2, h2);
		if (doing_multiview)
			draw_scene ();

		// Lower right
		glutSetWindow (subwindows [5]);
		glutPositionWindow (usable_x + 2*w2 + 2*interwindow_space, usable_y + h2 + interwindow_space);
		glutReshapeWindow (w2, h2);
		changesize (&cc_subwindows [5], w2, h2);
		if (doing_multiview)
			draw_scene ();
#endif
	}
}

//---------------------------------------------------------------------------
// Name:	draw_quad
// Purpose:	Take four sets of xyz coordinates and express them as QUAD corners
//---------------------------------------------------------------------------

void 
draw_quad (GLfloat x1, GLfloat y1, GLfloat z1,
		  GLfloat x2, GLfloat y2, GLfloat z2,
		  GLfloat x3, GLfloat y3, GLfloat z3,
		  GLfloat x4, GLfloat y4, GLfloat z4)
{	
	glColor4f (1.0,1.0,1.0,1.0);

	glBegin (GL_QUADS);
	
	glVertex3f (x1,y1,z1);
	glVertex3f (x2,y2,z2);
	glVertex3f (x3,y3,z3);
	glVertex3f (x4,y4,z4);

	glEnd ();
}

//---------------------------------------------------------------------------
// Name:	show_interline_distance 
// Purpose:	Displays some text telling what the inter-measuring line
//		distance is.
//---------------------------------------------------------------------------

void
show_interline_distance ()
{
	float diff = measuring_line_1 - measuring_line_2;
#if 0
	if (diff < 0.)
		diff *= -1.;
#endif

	char tmp [100];
	sprintf (tmp, "Distance between lines: %g mm", 1000. * diff);
	widget_status_line->set_text (tmp);
}

//---------------------------------------------------------------------------
// Name:	glui_line1_callback
// Porpoise:	Accepts user input to generate the new line position.
//---------------------------------------------------------------------------

void glui_line1_callback (const int control)
{
#if 0
	char tmp [100];
	sprintf (tmp, "line1_value = %g", line1_value);
	warning (tmp);
#endif

	measuring_line_1 = line1_value - initial_line1_value + our_starting_line1_value;
	recentest_line1_value = line1_value;

	show_interline_distance ();

	glutPostRedisplay ();
}


//---------------------------------------------------------------------------
// Name:	glui_line2_callback
// Porpoise:	Accepts user input to generate the new line position.
//---------------------------------------------------------------------------

void glui_line2_callback (const int control)
{
#if 0
	char tmp [100];
	sprintf (tmp, "line2_value = %g", line2_value);
	warning (tmp);
#endif

	measuring_line_2 = line2_value - initial_line2_value + our_starting_line2_value;
	recentest_line2_value = line2_value;

	show_interline_distance ();

	glutPostRedisplay ();
}


//---------------------------------------------------------------------------
// Name:	gui_line_dir_callback
// Purpose:	Toggles the direction of the measuring lines.
//---------------------------------------------------------------------------

void glui_line_dir_callback (const int control)
{
	which_measuring_line_direction = control;
	glutPostRedisplay ();
}

//---------------------------------------------------------------------------
// Name:	gui_cut_away_axis_callback
// Purpose:	Toggles the axis upon which the cut away feature operates.
//---------------------------------------------------------------------------

void 
glui_cut_away_axis_callback (const int control)
{
	cut_away_plane_location = 0.;
	widget_cutaway_trans->set_float_val (0.);

	cut_away_axis = (cut_away_axis + 1) % 6;
	which_measuring_line_direction = 1; // overjet

	//--------------------
	// Set plane equation
	//
	switch (cut_away_axis)
	{
		case 0 :
			cut_away_plane_equation[0] = 0;
			cut_away_plane_equation[1] = 0;
			cut_away_plane_equation[2] = -1;
			cut_away_plane_equation[3] = cut_away_plane_location;
			break;
		case 1 :
			cut_away_plane_equation[0] = 0;
			cut_away_plane_equation[1] = -1;
			cut_away_plane_equation[2] = 0;
			cut_away_plane_equation[3] = cut_away_plane_location;
			break;
		case 2 :
			cut_away_plane_equation[0] = -1;
			cut_away_plane_equation[1] = 0;
			cut_away_plane_equation[2] = 0;
			cut_away_plane_equation[3] = cut_away_plane_location;
			break;
		case 3 :
			cut_away_plane_equation[0] = 0;
			cut_away_plane_equation[1] = 0;
			cut_away_plane_equation[2] = 1;
			cut_away_plane_equation[3] = cut_away_plane_location;
			break;
		case 4 :
			cut_away_plane_equation[0] = 0;
			cut_away_plane_equation[1] = 1;
			cut_away_plane_equation[2] = 0;
			cut_away_plane_equation[3] = cut_away_plane_location;
			break;
		case 5 :
			cut_away_plane_equation[0] = 1;
			cut_away_plane_equation[1] = 0;
			cut_away_plane_equation[2] = 0;
			cut_away_plane_equation[3] = cut_away_plane_location;
			break;
	}

	glClipPlane (GL_CLIP_PLANE3, cut_away_plane_equation);

	draw_scene ();
}

//---------------------------------------------------------------------------
// Name:	gui_set_status
// Purpose:	Sets the position of the cut away plane via GLUI translation widget.
//---------------------------------------------------------------------------

void glui_cut_away_position_callback (const int control)
{
	cut_away_plane_equation[3] = cut_away_plane_location;
}


//---------------------------------------------------------------------------
// Name:	cut_away
// Purpose:	Clips model and caps with stenciled polygon to show cut away view.
//---------------------------------------------------------------------------

void cut_away ()
{
	if (!model)
		return;

	// clip stuff in front of plane
	glClipPlane (GL_CLIP_PLANE3, cut_away_plane_equation);
	glEnable (GL_CLIP_PLANE3);

	// don't send stuff to the color or depth buffer for now
	glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask (GL_FALSE);

	// enable stencil test and clear the stencil
	glEnable (GL_STENCIL_TEST);
	glClear (GL_STENCIL_BUFFER_BIT);

	// draw the back polygons to the stencil buffer
	glEnable (GL_CULL_FACE);
	glCullFace (GL_FRONT);\
	glStencilFunc (GL_ALWAYS, 0x1, 0x1);
	glStencilOp (GL_INCR,GL_INCR,GL_INCR);

	if (model) 
	{
		OpenGLRenderContext cxt;
		model->express (&cxt); 
	}

	// remove model front faces from stencil buffer
	glStencilFunc (GL_ALWAYS, 0x0, 0x1);
	glStencilOp (GL_DECR,GL_DECR,GL_DECR);
	glCullFace (GL_BACK);

	if (model) 
	{
		OpenGLRenderContext cxt;
		model->express (&cxt); 
	} 

	// don't change the stencil buffer anymore
	glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);

	// disable clip plane while we draw capping polygon
	glDisable (GL_CLIP_PLANE3);

	// draw a quad coplanar with the clip plane, wherever the stencil is non-zero
	glStencilFunc (GL_NOTEQUAL, 0x0, 0x1);
	glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask (GL_TRUE);

	switch (cut_away_axis)
	{
		case 0 : 
			draw_quad (.1,.1,cut_away_plane_location,-.1,.1,cut_away_plane_location,-.1,-.1,cut_away_plane_location,.1,-.1,cut_away_plane_location);
			break;
		case 1 :
			draw_quad (.1,cut_away_plane_location,-.1,-.1,cut_away_plane_location,-.1,-.1,cut_away_plane_location,.1,.1,cut_away_plane_location,.1);
			break;
		case 2 :
			draw_quad (cut_away_plane_location,.1,.1,cut_away_plane_location,-.1,.1,cut_away_plane_location,-.1,-.1,cut_away_plane_location,.1,-.1);
			break;
		case 3 : 
			draw_quad (.1,-.1,-cut_away_plane_location,-.1,-.1,-cut_away_plane_location,-.1,.1,-cut_away_plane_location,.1,.1,-cut_away_plane_location);
			break;
		case 4 :
			draw_quad (.1,-cut_away_plane_location,.1,-.1,-cut_away_plane_location,.1,-.1,-cut_away_plane_location,-.1,.1,-cut_away_plane_location,-.1);
			break;
		case 5 :
			draw_quad (-cut_away_plane_location,.1,-.1,-cut_away_plane_location,-.1,-.1,-cut_away_plane_location,-.1,.1,-cut_away_plane_location,.1,.1);
			break;
		default :
			break;
	}

	// flip stencil to mask everything except capping polygon
	glStencilFunc (GL_EQUAL, 0x0, 0x1);

	// disable stencil test, resume clipping
	glDisable (GL_STENCIL_TEST);
	glEnable (GL_CLIP_PLANE3);
}


//---------------------------------------------------------------------------
// Name:	set_rgb_color
//---------------------------------------------------------------------------
void
set_rgb_color (unsigned long color)
{
	GLfloat materialColor[4];
	materialColor [0] = 255==(0xff & (color>>16)) ? 1.f : 0.f;
	materialColor [1] = 255==(0xff & (color>>8)) ? 1.f : 0.f;
	materialColor [2] = 255==(0xff & color) ? 1.f : 0.f;
	materialColor [3] = 0.f; 
	glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
		materialColor);
	glMaterialfv (GL_FRONT, GL_EMISSION, materialColor);
}

//---------------------------------------------------------------------------
// Name:	draw_scene_inner
// Purpose:	Routine to construct the scene of objects and situate them,
//		and to finally request that they be drawn.
//---------------------------------------------------------------------------
void 
draw_scene_inner (CameraCharacteristics *cc)
{
	if (!showing_maxilla && !showing_mandible) {
		return; // SHOWING_NOTHING
	}
	if (cc->scale_factor == 0.)
		return;

	GLfloat save_fg[4];
	GLfloat save_bg[4];

	set_configuration_flags (cc);

	// If we're in multiview mode, the main window serves
	// as a backdrop to provide lines between the subwindows.
	//
	int curr_win = glutGetWindow ();
	if (doing_multiview && curr_win == main_window) {
			glClearColor (1.f, 1.f, 1.f, 1.f);
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glutSwapBuffers ();
			return;
	}

	if (!which_cross_section) {
		glDisable (GL_CLIP_PLANE1);
	} else {
		// For cross-section, the user-defined colors are not used.
		//
		for (int i=0; i<4; i++) {
			save_fg[i] = user_fg[i];
			save_bg[i] = user_bg[i];
			user_fg[i] = 0.f;
			user_bg[i] = 0.f;
		}
		user_bg[2] = 0.5f; // dark blue
		user_fg[0] = 1.f;  // red
	}

	if (redrawing_for_selection) {
#ifdef WIN32
		ZeroMemory (selection_buffer, SELECTION_BUFFER_SIZE * sizeof (GLuint));
#else
		bzero (selection_buffer, SELECTION_BUFFER_SIZE * sizeof (GLuint));
#endif
		glSelectBuffer (SELECTION_BUFFER_SIZE, selection_buffer);
		glRenderMode (GL_SELECT);
		glInitNames ();
		glPushName (-1);

	} else {
//		glPushAttrib (GL_ALL_ATTRIB_BITS);

		// Set up local viewer model.
#if 0
		glLightModeli (GL_LIGHT_MODEL_LOCAL_VIEWER, 
			!doing_orthographic ? 0 : 1); 
#endif
		
		if (using_custom_colors || !model)
			glClearColor (user_bg[0], user_bg[1], 
					user_bg[2], 1.0f);
		else
			glClearColor (model->bg[0], model->bg[1], 
					model->bg[2], 1.0f);

		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	glPushMatrix ();
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity (); // reset camera

	if (redrawing_for_selection) {
		GLint viewport[4];
		glGetIntegerv (GL_VIEWPORT, viewport);

#if 1
		printf ("Viewport: %d %d %d %d\n", 
			viewport[0], 
			viewport[1], 
			viewport[2], 
			viewport[3]);
#endif

		int x = mouse_x - viewport [0];
		int y = viewport[3] - mouse_y;

		//printf ("x,y= %d,%d\n", x, y);

		if (!showing_bolton)
			gluPickMatrix (x, y + 30, 1, 1, viewport); // Adjustment for top GLUI bar.
		else
			gluPickMatrix (x, y, 1, 1, viewport);
	}

	// N.B. The value of xy_aspect will be different for
	// subwindows than it will be for the main_window.
	//
	gluPerspective (doing_orthographic? 1.f : (using_preset_fieldofview? 45.f : cc->field_of_view),  // angle
		!doing_multiview ? xy_aspect : subwindow_aspect, // aspect ratio
		doing_orthographic ? 0.1f : 0.01f,  // near z clipping
		30.f); // far z clipping

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	glTranslatef (cc->viewpoints[0], cc->viewpoints[1], 0.f);

	if (!redrawing_for_selection)
		light0.express (0);

	if (doing_orthographic) {
		float len = .05f;
		glFrustum (-len, len, -len, len, 1.f, 0.0f);
		glTranslatef (0.0f, 0.0f, -1.2f);
	} else {
		glTranslatef (0.0f, 0.0f, -0.15f);
	}

	if (!which_cross_section) {
		// Zoom.
		float matrix[16];
		if (mouse_dragging) {
			Quat q;
			q.from_euler (cc->rotation_yaw, cc->rotation_pitch, 0.f, true);
			cc->combined_rotation.multiply (& cc->starting_rotation, &q);
			cc->combined_rotation.to_matrix4 (matrix);
		} else {
			cc->starting_rotation.to_matrix4 (matrix);
		}
		glMultMatrixf (matrix);

		glScalef (cc->scale_factor, cc->scale_factor, cc->scale_factor);
	}
	else
	{
		GLdouble a[4], b[4];
		int i;
		for (i=0; i<3; i++)
			a[i] = b[i] = 0.;

		// The position depends on the horizontal mouse
		// pointer position. To the far left you get the
		// minz value, to the far right you get maxz.
		//
		double position = ((double)mouse_x) / ((double)usable_width);
		if (model) {
			switch (which_cross_section) {
			case 'x':
				position *= model->maxx - model->minx;
				position += model->minx;
				a[0] = -1.;
				b[0] = 1.;
				glRotatef (-90.f, 0.f, 1.f, 0.f);
				break;
			case 'y':
				position *= model->maxy - model->miny;
				position += model->miny;
				a[1] = -1.;
				b[1] = 1.;
				glRotatef (90.f, 1.f, 0.f, 0.f);
				break;
			case 'z':
				position *= model->maxz - model->minz;
				position += model->minz;
				a[2] = -1.;
				b[2] = 1.;
				break;
			}
		}

		a[3] = position + 0.0003;
		b[3] = -position;

		glClipPlane (GL_CLIP_PLANE0, a);
		glClipPlane (GL_CLIP_PLANE1, b);

		glEnable (GL_CLIP_PLANE0);
		glEnable (GL_CLIP_PLANE1);

		glScalef (cc->scale_factor, cc->scale_factor, cc->scale_factor);
	}
	
	if (using_measuring_lines) {	
		glBegin (GL_LINES);
		glLineWidth (1.);

		set_rgb_color (RGB_RED);

		double loc = 0.0001 + cut_away_plane_location;

		switch (cut_away_axis) {
		case 0:
			if (!which_measuring_line_direction) {
				// Lines differ along X axis.
				glVertex3d (measuring_line_1, model->miny, loc);
				glVertex3d (measuring_line_1, model->maxy, loc);
				set_rgb_color (RGB_GREEN);
				glVertex3d (measuring_line_2, model->miny, loc);
				glVertex3d (measuring_line_2, model->maxy, loc);
			} else {
				// Lines differ along Y axis.
				glVertex3d (model->minx, measuring_line_1, loc);
				glVertex3d (model->maxx, measuring_line_1, loc);
				set_rgb_color (RGB_GREEN);
				glVertex3d (model->minx, measuring_line_2, loc);
				glVertex3d (model->maxx, measuring_line_2, loc);
			}
			break;
		case 1:
			if (!which_measuring_line_direction) {
				// Lines differ along X axis.
				glVertex3d (measuring_line_1, loc, model->minz);
				glVertex3d (measuring_line_1, loc, model->maxz);
				set_rgb_color (RGB_GREEN);
				glVertex3d (measuring_line_2, loc, model->minz);
				glVertex3d (measuring_line_2, loc, model->maxz);
			} else {
				// Lines differ along Z axis.
				glVertex3d (model->minx, loc, measuring_line_1);
				glVertex3d (model->maxx, loc, measuring_line_1);
				set_rgb_color (RGB_GREEN);
				glVertex3d (model->minx, loc, measuring_line_2);
				glVertex3d (model->maxx, loc, measuring_line_2);
			}
			break;
		case 2:
			if (!which_measuring_line_direction) {
				// Lines differ along Y axis.
				glVertex3d (loc, measuring_line_1, model->minz);// Default
				glVertex3d (loc, measuring_line_1, model->maxz);
				set_rgb_color (RGB_GREEN);
				glVertex3d (loc, measuring_line_2, model->minz);
				glVertex3d (loc, measuring_line_2, model->maxz);
			} else {
				// Lines differ along Z axis.
				glVertex3d (loc, model->miny, measuring_line_1);
				glVertex3d (loc, model->maxy, measuring_line_1);
				set_rgb_color (RGB_GREEN);
				glVertex3d (loc, model->miny, measuring_line_2);
				glVertex3d (loc, model->maxy, measuring_line_2);
			}
			break;
		case 3:
			if (!which_measuring_line_direction) {
				// Lines differ along Y axis.
				glVertex3d (model->minx, measuring_line_1, -loc);
				glVertex3d (model->maxx, measuring_line_1, -loc);
				set_rgb_color (RGB_GREEN);
				glVertex3d (model->minx, measuring_line_2, -loc);
				glVertex3d (model->maxx, measuring_line_2, -loc);
			} else {
				// Lines differ along X axis.
				glVertex3d (measuring_line_1, model->miny, -loc);
				glVertex3d (measuring_line_1, model->maxy, -loc);
				set_rgb_color (RGB_GREEN);
				glVertex3d (measuring_line_2, model->miny, -loc);
				glVertex3d (measuring_line_2, model->maxy, -loc);
			}
			break;
		case 4:
			if (!which_measuring_line_direction) {
				// Lines differ along X axis.
				glVertex3d (measuring_line_1, -loc, model->minz);
				glVertex3d (measuring_line_1, -loc, model->maxz);
				set_rgb_color (RGB_GREEN);
				glVertex3d (measuring_line_2, -loc, model->minz);
				glVertex3d (measuring_line_2, -loc, model->maxz);
			} else {
				// Lines differ along Z axis.
				glVertex3d (model->minx, -loc, measuring_line_1);
				glVertex3d (model->maxx, -loc, measuring_line_1);
				set_rgb_color (RGB_GREEN);
				glVertex3d (model->minx, -loc, measuring_line_2);
				glVertex3d (model->maxx, -loc, measuring_line_2);
			}
			break;
		case 5:
			if (!which_measuring_line_direction) {
				// Lines differ along Y axis.
				glVertex3d (-loc, measuring_line_1, model->minz);
				glVertex3d (-loc, measuring_line_1, model->maxz);
				set_rgb_color (RGB_GREEN);
				glVertex3d (-loc, measuring_line_2, model->minz);
				glVertex3d (-loc, measuring_line_2, model->maxz);
			} else {
				// Lines differ along Z axis.
				glVertex3d (-loc, model->miny, measuring_line_1);
				glVertex3d (-loc, model->maxy, measuring_line_1);
				set_rgb_color (RGB_GREEN);
				glVertex3d (-loc, model->miny, measuring_line_2);
				glVertex3d (-loc, model->maxy, measuring_line_2);
			}
			break;
		}
		glEnd ();
	}

	if (doing_cut_away) 
		cut_away (); 

	if (model) 
	{
		OpenGLRenderContext cxt;
		model->express (&cxt); 
	}

	glPopMatrix ();

	if (redrawing_for_selection) {
		glPopName ();
		glFlush ();
		selection_buffer_count = glRenderMode (GL_RENDER);
	}
	else {
		glutSwapBuffers ();
//		glPopAttrib ();
	}
	
	if (which_cross_section) {
		for (int i=0; i<4; i++) {
			user_fg [i] = save_fg[i];
			user_bg [i] = save_bg[i];
		}
	}

	if (doing_cut_away)
		glDisable (GL_CLIP_PLANE3);
}

//---------------------------------------------------------------------------
// Name:	draw_scene
// Purpose:	Routine to construct the scene of objects and situate them,
//		and to finally request that they be drawn.
//---------------------------------------------------------------------------

void 
draw_scene ()
{
	if (doing_multiview) {
		CameraCharacteristics *cc = get_pertinent_cc ();
		draw_scene_inner (cc);
	} else {
		glutSetWindow (showing_bolton ? 
				secondary_window : main_window);
		draw_scene_inner (&cc_main);
	}
}

//---------------------------------------------------------------------------
// Name:	init_renderer 
// Purpose:	Does some initialization.
//---------------------------------------------------------------------------
void 
init_renderer () 
{
	glEnable (GL_DEPTH_TEST);  // Enable Z buffering.
	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);
	glEnable (GL_NORMALIZE);   // Ensures normals unaffected by glScale
	glEnable (GL_LINE_SMOOTH);

	// glShadeModel (GL_SMOOTH);  // Gouraud shading.
	glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

//	glColorMaterial (GL_FRONT, GL_DIFFUSE);

	// Note that COLOR_MATERIAL is *not* enabled.

	// Set the background color to start with.
	glClearColor (user_bg[0], user_bg[1], user_bg[2], 1.0f);
	
	light0.express (0);
}


#ifdef ORTHOCAST

void
find_maxilla_mandible (IndexedFaceSet* &ifs1, IndexedFaceSet* &ifs2)
{
	ifs1 = ifs2 = NULL;

	if (!model)
		return;

	Group* g = model->top_and_bottom_node;
	Replicated *r1;
	Replicated *r2;
	Transform *t1;
	Transform *t2;

	if (!g)
		return;

	r1 = (Replicated*) g->children;
	r2 = (Replicated*) g->children->next;

	if (strncmp (r1->type, "Replic", 6) ||
		strncmp (r2->type, "Replic", 6)) {
			return;
	}
	
	//------------------------------------------------------------
	// Determine which is the mandible by looking for name 
	// "BottomSet".
	//
	t1 = (Transform*) r1->original;
	t2 = (Transform*) r2->original;			
	if (strncmp (t1->type, "Transf", 6) ||
		strncmp (t2->type, "Transf", 6)) {
			warning ("DST data is not organized as expected, cannot offer Open view.");
			return;
	}
	if (!strncmp ("BottomSet", t2->name, 9)) {
		// As expected.
	} else {
		Transform *tmp = t1;
		t1 = t2;
		t2 = tmp;
	}

	//------------------------------------------------------------
	// Establish the occlusal view.
	//
	ifs1 = (IndexedFaceSet*) find_node_by_type (t1, "IndexedFaceSet");
	ifs2 = (IndexedFaceSet*) find_node_by_type (t2, "IndexedFaceSet");
}

//---------------------------------------------------------------------------
// Name:	construct_occlusal_kludge 
// Purpose:	Makes up for insufficient VRML in DST file,
//		specifically the lack of an occlusal Switch
//		in DST files.
//---------------------------------------------------------------------------
static bool 
construct_occlusal_kludge () 
{
	if (!model)
		return false;
	if (!model->top_and_bottom_node)
		return false;
	
	// We must create on the fly an additional
	// Switch option.
	//
	Switch *sw = (Switch*) model->main_switch;
	if (sw) {
		sw->which = 0;
		sw->update_which_node ();
	} else
		return false;

	IndexedFaceSet *ifs1;
	IndexedFaceSet *ifs2;
	find_maxilla_mandible (ifs1, ifs2);

	if (!ifs1 || !ifs2) {
		warning ("DST data is not organized as expected, cannot offer Open view.");
		return false;
	}

	Transform *top = new Transform ();
	Transform *bottom = new Transform ();

	float top_width = ifs1->maxx - ifs1->minx;
	float top_height = ifs1->maxy - ifs1->miny;
	top_depth = ifs1->maxz - ifs1->minz;
	top->translate_x = -ifs1->minx - top_width/2.f;
	top->translate_y = -ifs1->miny - top_height/2.f;
	top->translate_z = -ifs1->minz - top_depth/2.f;
	top->operations[top->op_index++] = Transform::TRANSLATE;

	float bottom_width = ifs2->maxx - ifs2->minx;
	float bottom_height = ifs2->maxy - ifs2->miny;
	bottom_depth = ifs2->maxz - ifs2->minz;
	bottom->translate_x = -ifs2->minx - bottom_width/2.f;
	bottom->translate_y = -ifs2->miny - bottom_height/2.f;
	bottom->translate_z = -ifs2->minz - bottom_depth/2.f;
	bottom->operations[bottom->op_index++] = Transform::TRANSLATE;

	if(!model->isTopAndBottomAligned)
	{
		top->second_rotate_angle = -M_PI;
		top->second_rotate_x = 0.f;
		top->second_rotate_y = 0.f;
		top->second_rotate_z = 1.f;
		top->operations[top->op_index++] = Transform::ROTATE2;	
	}

	top->rotate_angle = -M_PI / 2.f;
	top->rotate_x = 1.f;
	top->rotate_y = 0.f;
	top->rotate_z = 0.f;
	top->operations[top->op_index++] = Transform::ROTATE;

	bottom->rotate_angle = M_PI / 2.f;
	bottom->rotate_x = 1.f;
	bottom->rotate_y = 0.f;
	bottom->rotate_z = 0.f;		
	bottom->operations[bottom->op_index++] = Transform::ROTATE;

	top->second_translate_x = 0.f;
	top->second_translate_y = (occlusal_space + top_depth)/2.f;
	top->second_translate_z = top_height;
	top->operations[top->op_index++] = Transform::TRANSLATE2;

	bottom->second_translate_x = 0.f;
	bottom->second_translate_y = (occlusal_space + bottom_depth) / -2.f;
	bottom->second_translate_z = bottom_height;
	bottom->operations[bottom->op_index++] = Transform::TRANSLATE2;

	occlusal_top = top;
	occlusal_bottom = bottom;

#ifdef WIN32
	top->name = _strdup ("occlusal_top");
	bottom->name = _strdup ("occlusal_bottom");
#else
	top->name = strdup ("occlusal_top");
	bottom->name = strdup ("occlusal_bottom");
#endif
	model->add_name_mapping (top->name, top);
	model->add_name_mapping (bottom->name, bottom);

	top->add_child (ifs1);
	bottom->add_child (ifs2);

	// Use these to prevent infinite loop.
	Replicated *rtop = new Replicated (model, top->name);
	Replicated *rbot = new Replicated (model, bottom->name);

	Group *g_new = new Group ();
	g_new->add_child (rtop);
	g_new->add_child (rbot);

	occlusal_index = sw->add_child (g_new) - 1;

	printf ("#1: Width %g Height %g Depth %g\n", 
		ifs1->maxx - ifs1->minx,
		ifs1->maxy - ifs1->miny,
		ifs1->maxz - ifs1->minz);

	printf ("#2: Width %g Height %g Depth %g\n", 
		ifs2->maxx - ifs2->minx,
		ifs2->maxy - ifs2->miny,
		ifs2->maxz - ifs2->minz);

	return true;
}

//---------------------------------------------------------------------------
// Name:	construct_manual_spacing_kludge 
// Purpose:	adds a switch to allow for manual spacing
//---------------------------------------------------------------------------

#define MAX(a,b) (a > b ? a : b)
#define MIN(a,b) (a < b ? a : b)

static bool 
construct_manual_spacing_kludge ()
{
	if (!model || !model->top_and_bottom_node)
		return false;
	
	// We must create on the fly an additional Switch option.
	//
	Switch *sw = (Switch*) model->main_switch;
	if (sw) {
		sw->which = 0;
		sw->update_which_node ();
	} else
		return false;

	IndexedFaceSet *ifs1;
	IndexedFaceSet *ifs2;
	find_maxilla_mandible (ifs1, ifs2);

	if (!ifs1 || !ifs2) {
		warning ("DST data is not organized as expected, cannot offer manual spacing...");
		return false;
	}

	Transform *top = new Transform ();
	Transform *bottom = new Transform ();

	top->dont_save_this_node = true;
	bottom->dont_save_this_node = true;

	// Setup maxilla transformation

	double top_width = ifs1->maxx - ifs1->minx;
	double top_height = ifs1->maxy - ifs1->miny;
	double top_depth = ifs1->maxz - ifs1->minz;

	double bottom_width = ifs2->maxx - ifs2->minx;
	double bottom_height = ifs2->maxy - ifs2->miny;
	double bottom_depth = ifs2->maxz - ifs2->minz;

	// #######  TODO (Kris): these need to be determined somehow

	if(!model->isTopAndBottomAligned) 
	{	
		float bite_adjustment_x = -0.001;
		float bite_adjustment_y = 0.005;
		float bite_adjustment_z = 0;

		top->translate_x = -ifs1->minx - top_width/2.f + bite_adjustment_x;
		top->translate_y = -ifs1->miny - top_height + bite_adjustment_y;
		top->translate_z = -ifs1->minz + bite_adjustment_z;
		top->operations[top->op_index++] = Transform::TRANSLATE;

		top->rotate_angle = -M_PI;
		top->rotate_x = 0.f;
		top->rotate_y = 0.f;
		top->rotate_z = 1.f;
		top->operations[top->op_index++] = Transform::ROTATE;	
	}	
	

	// Setup mandible transformation
	if(!model->isTopAndBottomAligned)
	{	
		bottom->translate_x = -ifs2->minx - bottom_width/2.f;
		bottom->translate_y = -ifs2->miny - bottom_height;
		bottom->translate_z = -ifs2->minz;
		bottom->operations[bottom->op_index++] = Transform::TRANSLATE;

		bottom->rotate_angle = 0.f;
		bottom->rotate_x = 0.f;
		bottom->rotate_y = 0.f;
		bottom->rotate_z = 0.f;
		bottom->operations[bottom->op_index++] = Transform::ROTATE;
	}
	else
	{
		bottom->translate_x = -ifs2->minx - bottom_width/2.f;
		bottom->translate_y = -ifs2->miny - bottom_height/2.f;
		bottom->translate_z = -ifs2->minz;
		bottom->operations[bottom->op_index++] = Transform::TRANSLATE;	

		bottom->rotate_angle = 0.f;
		bottom->rotate_x = 0.f;
		bottom->rotate_y = 0.f;
		bottom->rotate_z = 0.f;
		bottom->operations[bottom->op_index++] = Transform::ROTATE;

		bottom->second_translate_x = -bottom->translate_x;
		bottom->second_translate_y = -bottom->translate_y;
		bottom->second_translate_z = -bottom->translate_z;
		bottom->operations[bottom->op_index++] = Transform::TRANSLATE2;
	}

	manual_spacing_top = top;
	manual_spacing_bottom = bottom;

	// Other stuff

#ifdef WIN32
	top->name = _strdup ("manual_spacing_top");
	bottom->name = _strdup ("manual_spacing_bottom");
#else
	top->name = strdup ("manual_spacing_top");
	bottom->name = strdup ("manual_spacing_bottom");
#endif

	model->add_name_mapping (top->name, top);
	model->add_name_mapping (bottom->name, bottom);

	top->add_child (ifs1);
	bottom->add_child (ifs2);

	// Use these to prevent infinite loop.
	Replicated *rtop = new Replicated (model, top->name);
	Replicated *rbot = new Replicated (model, bottom->name);

	Group *g_new = new Group ();
	g_new->add_child (rtop);
	g_new->add_child (rbot);

	g_new->dont_save_this_node = true;
	rtop->dont_save_this_node = true;
	rbot->dont_save_this_node = true;

	manual_spacing_index = sw->add_child (g_new) - 1;

	return true;
}
#endif

//---------------------------------------------------------------------------
// Name:	set_examination_angles
// Purpose:	Sets the yaw and pitch at which the model will be examined.
//---------------------------------------------------------------------------
void 
set_examination_angles (CameraCharacteristics *cc, const float yaw, const float pitch)
{
	cc->starting_rotation.from_euler (yaw, pitch, 0.f, true);
}

//---------------------------------------------------------------------------
// Name:	glui_view_callback
// Purpose:	Callback function to handle a change of the view.
//		Can be invoked either via Listbox or Buttons.
//---------------------------------------------------------------------------
void 
glui_view_callback (const int control)
{
	CameraCharacteristics *cc = get_pertinent_cc ();

	if (current_view == VIEW_X_CROSS_SECTION || 
		current_view == VIEW_Y_CROSS_SECTION || 
		current_view == VIEW_Z_CROSS_SECTION)
		stop_cross_section ();

	current_view = control;

	switch (control) {
	case VIEW_FRONT:
		which_cross_section = 0;
		light0.reset (doing_orthographic ? 'o' : 'n');
		set_examination_angles (cc, 0.f, 0.f);
		glutPostRedisplay ();
		break;
	case VIEW_BACK:
		which_cross_section = 0;
		light0.reset (doing_orthographic ? 'o' : 'n');
		set_examination_angles (cc, 180.f, 0.f);
		glutPostRedisplay ();
		break;
	case VIEW_LEFT:
		which_cross_section = 0;
		light0.reset (doing_orthographic ? 'o' : 'n');
		set_examination_angles (cc, 90.f, 0.f);
		glutPostRedisplay ();
		break;
	case VIEW_RIGHT:
		which_cross_section = 0;
		light0.reset (doing_orthographic ? 'o' : 'n');
		set_examination_angles (cc, -90.f, 0.f);
		glutPostRedisplay ();
		break;
	case VIEW_TOP:
		which_cross_section = 0;
		light0.reset (doing_orthographic ? 'o' : 'n');
		if (input_file && input_file->is_dst_file && showing_maxilla) {
			set_examination_angles (cc, 0.f, 270.f);
		} else {
			set_examination_angles (cc, 90.f, 180.f);
		}
		glutPostRedisplay ();
		break;	
	case VIEW_X_CROSS_SECTION:
		which_cross_section = 'x';
		set_examination_angles (cc, 90.f, 0.f);
		init_cross_section ();
		glutPostRedisplay ();
		break;
	case VIEW_Y_CROSS_SECTION:
		which_cross_section = 'y';
		set_examination_angles (cc, 0.f, 270.f);
		init_cross_section ();
		glutPostRedisplay ();
		break;
	case VIEW_Z_CROSS_SECTION:
		which_cross_section = 'z';
		set_examination_angles (cc, 0.f, 0.f);
		init_cross_section ();
		glutPostRedisplay ();
		break;
	}
}

//---------------------------------------------------------------------------
// Name:	glui_user_colors_callback
// Purpose:	Callback function to handle checkbox activity.
//---------------------------------------------------------------------------
void 
glui_user_colors_callback (const int control)
{
	bool checkvalue = widget_colors->get_int_val () ? true : false;
	using_custom_colors = checkvalue;
	draw_scene ();
	draw_scene ();
}



void
occlusal_callback (const int foo)
{
	if (!model)
		return;
	if (!input_file->is_dst_file) {
		warning ("File does not support occlusal view.");
		return;
	}
	if (doing_cut_away || doing_manual_spacing || doing_multiview) {
//		widget_occlusal->set_int_val (false); // no longer a checkbox
		return;
	}

	Switch *sw = (Switch*) model->main_switch;
	if (!sw)
		return;

	sw->which = 0;
	sw->which_node = NULL;

	CameraCharacteristics *cc = get_pertinent_cc ();

	showing_occlusal = !showing_occlusal;

	model->show_occlusal2 = false;

	// If showing occlusal was checked
	//
	if (showing_occlusal)
	{
		if (!input_file->is_dst_file) {
			warning ("Model is not a DST file.");
			showing_occlusal = false;
			return;
		} else if (!have_occlusal) {
			warning ("DST data is not organized as expected, cannot offer Open view.");
			showing_occlusal = false;
			return;
		}

		if (widget_multiview) 
			widget_multiview->disable ();
		widget_cutaway->disable ();
		widget_manual_spacing->disable ();
		widget_mandible->disable ();
		widget_maxilla->disable ();
		widget_mandible->set_int_val (1);
		widget_mandible->set_int_val (1);
		showing_mandible = true;
		showing_maxilla = true;
		cc_main.configuration = SHOW_BOTH;

		// Recenter in case only mandible or only maxilla was shown.
		if (sw) {
			// Centering occlusal is tricky!
			// First center it as non-occlusal.
			sw->which = 0;
			sw->doing_open_view = false;
			sw->update_which_node ();
			model->autocenter ();

			sw->which = occlusal_index;
			sw->doing_open_view = true;
			sw->update_which_node ();
			// Set the zoom based on the newly ascertained model dimensions.
			auto_set_zoom (cc);

			cc->rotation_pitch = 0.0f;
			cc->rotation_yaw = 0.0f;

			cc->need_recenter = false;		

			glutPostRedisplay ();
		}
	} 		
	else 
	{
		//----------------------------------
		// Showing occlusal was disabled.
		//
		if (widget_multiview) 
			widget_multiview->enable ();
		widget_cutaway->enable ();
		widget_manual_spacing->enable ();
		widget_mandible->enable ();
		widget_maxilla->enable ();
		widget_maxilla->set_int_val (1);
		widget_mandible->set_int_val (1);
		showing_mandible = true;
		showing_maxilla = true;
		cc->configuration = SHOW_BOTH;
		cc->need_recenter = true;

		if (sw) {
			sw->which = 0;
			sw->which_node = NULL;
			sw->doing_open_view = false;
			sw->update_which_node ();

			glutPostRedisplay ();
		}
	}
}

void occlusal2_callback(const int foo)
{
	occlusal_callback(foo);
		
	model->show_occlusal2 = showing_occlusal;		
}

//---------------------------------------------------------------------------
// Name:	glui_checkbox_callback
// Purpose:	Callback function to handle checkbox activity.
//---------------------------------------------------------------------------
void 
glui_checkbox_callback (const int control)
{
	if (!model) 
		return;

	CameraCharacteristics *cc = get_pertinent_cc ();
	
	bool widget_cutaway_checkvalue = false;
	bool widget_manual_spacing_checkvalue = false;
	bool widget_show_maxilla = false;
	bool widget_show_mandible = false;
	bool need_recenter = false;

#ifndef ORTHOCAST
	checkvalue = widget_ortho->get_int_val () ? true : false;
	light0.reset (checkvalue ? 'o' : 'n');
	doing_orthographic = checkvalue;

	checkvalue = widget_fieldofview45->get_int_val () ? true : false;
	using_preset_fieldofview = checkvalue;

	checkvalue = widget_autocenter->get_int_val () ? true : false;
	doing_autocenter = checkvalue;

	checkvalue = widget_cross_section_x->get_int_val () ? true : false;
	which_cross_section = 0;
	if (checkvalue)
		which_cross_section = 'x';
	checkvalue = widget_cross_section_y->get_int_val () ? true : false;
	if (checkvalue)
		which_cross_section = 'y';
	checkvalue = widget_cross_section_z->get_int_val () ? true : false;
	if (checkvalue)
		which_cross_section = 'z';
	if (which_cross_section)
		init_cross_section ();
#endif 

#ifdef ORTHOCAST
	widget_cutaway_checkvalue = widget_cutaway->get_int_val () ? true : false;
	widget_manual_spacing_checkvalue = widget_manual_spacing->get_int_val () ? true : false;
	widget_show_maxilla = widget_maxilla->get_int_val () ? true : false;
	widget_show_mandible = widget_mandible->get_int_val () ? true : false;

	if (doing_cut_away != widget_cutaway_checkvalue) {
		//----------------------------------------
		// RULE: You cannot combine cutaway, 
		// occlusal, manual spacing or multiview.
		//
		if (doing_manual_spacing || doing_multiview || showing_occlusal) {
			widget_cutaway->set_int_val (false);
			return;
		}

		doing_cut_away = widget_cutaway_checkvalue;
		if (doing_cut_away) {	
			if (widget_multiview) 
				widget_multiview->disable ();
			widget_manual_spacing->disable ();
			widget_mandible->disable ();
			widget_maxilla->disable ();

			using_measuring_lines = true;

			//------------------------------
			// Whether starting or ending cut-away mode,
			// we reset the line positions.
			//
			glui_line_measuring_callback (0);

		} else {
			if (widget_multiview) 
				widget_multiview->enable ();
			widget_manual_spacing->enable ();
			widget_mandible->enable ();
			widget_maxilla->enable ();
			using_measuring_lines = false;
		}

		widget_maxilla->set_int_val (true);
		widget_mandible->set_int_val (true);

		cc_main.configuration = SHOW_BOTH;
		cc_main.need_recenter = true;

		glDisable (GL_CLIP_PLANE3);
		draw_scene ();
		return;
	}

	if (doing_manual_spacing != widget_manual_spacing_checkvalue) 
	{
		//----------------------------------------
		// RULE: You cannot combine cutaway and 
		// manual spacing or multiview.
		//
		if (doing_cut_away || doing_multiview || showing_occlusal) {
			widget_manual_spacing->set_int_val (false);
			return;
		}

		//--------------------------------------------------
		// Cannot do manual spacing if input was not DST file,
		// or if the both upper & lower are not being shown.
		//
		if (widget_manual_spacing_checkvalue && !input_file->is_dst_file) {
			widget_manual_spacing->set_int_val (false);
			warning ("File does not support manual spacing.");
			return;
		}
		if (widget_manual_spacing_checkvalue && !have_manual_spacing) {
			widget_manual_spacing->set_int_val (false);
			warning ("DST data is not organized as expected, cannot offer manual spacing.");
			return;
		} 

		doing_manual_spacing = widget_manual_spacing_checkvalue;

		Switch *sw = (Switch*) model->main_switch;
		if (!sw) {
			widget_cutaway->set_int_val (false);
			warning ("File does not support manual spacing.");
			doing_manual_spacing = false;
			return;
		}

		if (doing_manual_spacing) {	
			if (widget_multiview) 
				widget_multiview->disable ();
			widget_cutaway->disable ();
			widget_mandible->disable ();
			widget_maxilla->disable ();
			widget_mandible->set_int_val (1);
			widget_mandible->set_int_val (1);
			widget_mandible->disable ();
			widget_maxilla->disable ();
		} else {
			if (widget_multiview) 
				widget_multiview->enable ();
			widget_cutaway->enable ();
			widget_mandible->enable ();
			widget_maxilla->enable ();
			widget_mandible->set_int_val (1);
			widget_mandible->set_int_val (1);
			widget_mandible->enable ();
			widget_maxilla->enable ();
		}

		if (doing_manual_spacing)
		{
			// Set the zoom based on the newly ascertained model dimensions.
			auto_set_zoom (cc);

			sw->which = manual_spacing_index;
			sw->which_node = NULL;
			sw->doing_open_view = false;
			sw->update_which_node ();

			cc->rotation_pitch = 0.0f;
			cc->rotation_yaw = 0.0f;

			showing_mandible = true;
			showing_maxilla = true;	
			cc_main.configuration = SHOW_BOTH;
			cc_main.need_recenter = true;

			widget_maxilla->set_int_val (true);
			widget_mandible->set_int_val (true);
			
			// Recenter in case only mandible or only maxilla was shown.
			//
			model->autocenter ();

		} else {
			sw->which = 0;
			sw->which_node = NULL;

			sw->doing_open_view = false;
			sw->update_which_node ();

			showing_mandible = true;
			showing_maxilla = true;

			widget_maxilla->set_int_val (true);
			widget_mandible->set_int_val (true);
			
			cc_main.configuration = SHOW_BOTH;
			cc_main.need_recenter = true;

			for (int i = 0; i < N_SUBWINDOWS; i++) {
				cc_subwindows[i].configuration = SHOW_BOTH;
				cc_subwindows[i].need_recenter = true;
			}
		}

		glutPostRedisplay ();
		return;
	}

	//------------------------------------------------------
	// Check whether show maxilla/mandible checkbox changed.
	//
	if (doing_multiview || showing_occlusal || doing_manual_spacing || doing_cut_away)
		return;
	switch (cc_main.configuration) {
	case SHOW_BOTH:
		showing_maxilla = true;
		showing_mandible = true;
		break;
	case SHOW_MAXILLA:
		showing_maxilla = true;
		showing_mandible = false;
		break;
	case SHOW_MANDIBLE:
		showing_maxilla = false;
		showing_mandible = true;
		break;
	}

	bool changed = false;
	if (widget_show_maxilla != showing_maxilla) {
		changed = true;
	}
	else if (widget_show_mandible != showing_mandible) {
		changed = true;
	}
	if (changed) {
		showing_maxilla = widget_show_maxilla;
		showing_mandible = widget_show_mandible;
		if (!showing_mandible && !showing_maxilla) {
			showing_maxilla = showing_mandible = true;
		}
		if (showing_mandible && showing_maxilla)
			cc_main.configuration = SHOW_BOTH;
		else if (showing_maxilla && !showing_mandible)
			cc_main.configuration = SHOW_MAXILLA;
		else
			cc_main.configuration = SHOW_MANDIBLE;

		cc_main.need_recenter = true;
		draw_scene ();
	}
#endif
}

//---------------------------------------------------------------------------
// Name:	InputFile::InputFile
// Purpose:	Create an InputFile object and locate filename within the path.
//---------------------------------------------------------------------------
InputFile::InputFile (char *path_) 
{
	ASSERT_NONZERO (path_,"path")
	//----------

	valid = true;
	is_dst_file = false;
	first_line = NULL;
	ungot_char = EOF;
	gzfile = NULL;
	tree = NULL;
	buffer_ix = buffer_limit = -1;
#ifdef WIN32
	path = _strdup (path_);
#else
	path = strdup (path_);
#endif
	total_allocated += strlen (path) + 1;
	name = strrchr (path, '/');
	extension = strrchr (path, '.');

	printf ("File extension is %s\n", extension? extension : " (none)");

	if (!extension)
		extension = "";

	//----------------------------------------
	// Do a simple check for a DST file.
	// This is the first check, a better one 
	// is done later.
	//
	if (!strcmp (extension, ".dst") || !strcmp (extension, ".DST")) {
		is_dst_file = true;
	}
	else
	if (!strcmp (extension, ".gz") || !strcmp (extension, ".GZ")) {
		extension--;
		while (extension >= path && *extension != '.')
			extension--;
	}
	else
	if (!strcmp (extension, ".vrml") || !strcmp (extension, ".VRML") ||
		!strcmp (extension, ".wrl") || !strcmp (extension, ".WRL") ||
		!strcmp (extension, ".vrm") || !strcmp (extension, ".VRM")) {
	} else {
		free (path);
		path = extension = NULL;
		valid = false;
		return;
	}

#ifdef WIN32
	if (!name)
		name = strrchr (path, '\\');
#endif
	if (name)
		name++;
}

//---------------------------------------------------------------------------
// Name:	close_file
// Purpose:	Closes file specified by current_input_file, update GUI.
//---------------------------------------------------------------------------
void
close_file ()
{
	if (!input_file)
		return;

	CameraCharacteristics *cc = get_pertinent_cc ();

	delete input_file;
	input_file = NULL;

	if (model) 
		delete model;
	model = NULL;

	gui_reset (true);
	gui_update_perspective ();

#ifndef ORTHOCAST
	widget_filename->set_text ("File: -");
	gui_set_title ();
	widget_field_of_view->set_text ("Field of View: -");
	gui_set_status ("");
#endif

	gui_set_window_title (NULL, NULL);
}

//---------------------------------------------------------------------------
// Name:	load_file
// Purpose:	Loads file at current_input_file, updates GUI.
//---------------------------------------------------------------------------
void
load_file (InputFile *file)
{
	char tmp[300];

	did_manual_spacing = false;

	ASSERT_NONZERO (file,"file")
	//----------

	if (input_file)
		close_file ();

#ifdef WIN32
	unsigned long t0, t, t2;
#endif

#ifdef WIN32
	t0 = millisecond_time ();
#endif
	InputWord *word_tree = vrml_reader (file);
#ifdef WIN32
	t = millisecond_time ();
	t -= t0;
#endif
	if (word_tree) {
#ifdef WIN32
		t0 = millisecond_time ();
#endif
		model = vrml_parser (word_tree);
		if (!model) {
			sprintf (tmp, "Unable to read file %s.", file->name);
			warning (tmp);
			model = NULL;
			return;
		}
		model->inputfile = file;

		//--------------------------------------------------
		// Look for the presence of a Switch node, which
		// an upper/lower-only .wrl file would not have,
		// but which a DST file would.
		//
		if (model->nodes && model->nodes->find_by_type ("Switch")) {
			file->is_dst_file = true;
		}

		//----------------------------------------
		// If the read was successful, reset GUI
		// to a known state.
		//
		gui_reset (true);
#if 0
		FILE *f1 = fopen ("c:/maxilla_words.txt", "wb");
		if (!f1) 
			f1 = stdout;
		inputword_dump (f1, word_tree);
		if (f1 != stdout)
			fclose (f1);

		FILE *f = fopen ("maxilla_nodes.txt", "wb");
		if (!f) 
			f = stdout;
		node_dump (f, model->nodes);
		if (f != stdout)
			fclose (f);
#endif

#ifdef ORTHOCAST
		if (model->case_number) {
			widget_case_number->set_text (model->case_number);
		}
		if (model->case_date) {
			widget_case_date->set_text (model->case_date);
		}
		if (model->control_number) {
			widget_control_number->set_text (model->control_number);
		}
		if (model->patient_birthdate) {
			widget_patient_birth_date->set_text (model->patient_birthdate);
		}

		char fname [200];
		char lname [200];
		memset(fname, 0 ,200);
		memset(lname, 0, 200);
		if (model->patient_firstname) 
			strcat (fname, model->patient_firstname);
		if (model->patient_lastname)
			strcpy (lname, model->patient_lastname);
		
		// If name entered as Last,First in last-name field:
		char *s = strchr (lname, ',');
		if (s) {
			*s++ = 0;
			while (*s && isspace ((int) *s))
				s++;
			if (*s)
				strcpy (fname, s);
		}
		// If name entered as Last,First in first-name field:
		s = strchr (fname, ',');
		if (s) {
			*s++ = 0;
			strcpy (lname, fname);
			char *s2 = fname;
			while (*s)
				*s2++ = *s++;
			*s2 = 0;
		}

		widget_patient_last_name->set_text (lname);
		widget_patient_first_name->set_text (fname);
#endif
#ifdef WIN32
		t2 = millisecond_time () - t0;
		printf ("Time to read & parse VRML: %g s\n", (t+t2) / 1000.0f);
#endif
		if (model) {
			model->word_tree = word_tree;
		} else {
			sprintf (tmp, "Unable to parse file %s.", file->name);
			warning (tmp);
			delete word_tree;
			return;
		}
	} else {
		sprintf (tmp, "Unable to read file %s.", file->name);
		warning (tmp);
		model = NULL;
		return;
	}

	// Print the VRML version
	// XX use sscanf to parse version#.
	if (file->first_line && '#'==*file->first_line)
		printf ("Header: %s", 1 + file->first_line);

	// Generate a window title.
	gui_set_window_title (model? model->title : NULL, model? file->name : NULL);

	// Set orthographic as default perspective.
	// XX cmdline switch?
	doing_orthographic = true;
	gui_update_perspective ();

	gui_set_title ();

#ifndef ORTHOCAST
	sprintf (tmp, "File: %s", file->name);
	widget_filename->set_text (tmp);

	sprintf (tmp, "Field of View: %g degrees", field_of_view);
	widget_field_of_view->set_text (tmp);
#endif

	if (doing_autocenter)
		model->autocenter ();

	input_file = file;

	sprintf (tmp, "Loaded file '%s'.\n", file->name);
	gui_set_status (tmp);

#ifdef ORTHOCAST
	if (input_file->is_dst_file)
	{
		have_manual_spacing = construct_manual_spacing_kludge ();
		have_occlusal = construct_occlusal_kludge ();
		widget_maxilla->enable ();
		widget_mandible->enable ();
		widget_manual_spacing->enable ();
	}
	else
	{
		have_occlusal = false;
		have_manual_spacing = false;
		widget_maxilla->disable ();
		widget_mandible->disable ();
		widget_manual_spacing->disable ();
	}
#endif

	printf ("After loading file, total memory use w/out deallocations = ");
	if (total_allocated < 1000000L) {
		float tmp = (float)total_allocated / 1.0E3f;
		printf ("%.4g kB\n", tmp);
	} else {
		float tmp = ((float)total_allocated) / 1.0E6f;
		printf ("%.4g MB\n", tmp);
	}

	//----------------------------------------
	// Look for the manual spacing parameters,
	// and read them if present.
	//
	clear_manual_spacing ();
	Text *text = (Text*) model->nodes->find_by_name ("manual_spacing");
	if (text && text->text) {
		float values [7];
		int count = sscanf (text->text, 
				"%g %g %g %g %g %g %g "
				"%g %g %g %g %g %g "
				"%g %g %g %g %g %g %g %g "
				"%g %g %g %g %g %g %g %g ",
			& values [0],
			& values [1],
			& values [2],
			& values [3],
			& values [4],
			& values [5],
			& values [6],
			& manual_spacing_viewpoints [0],
			& manual_spacing_viewpoints [1],
			& manual_spacing_viewpoints [2],
			& manual_spacing_viewpoints [3],
			& manual_spacing_viewpoints [4],
			& manual_spacing_viewpoints [5],
			& manual_spacing_rotation [0],
			& manual_spacing_rotation [1],
			& manual_spacing_rotation [2],
			& manual_spacing_rotation [3],
			& manual_spacing_rotation [4],
			& manual_spacing_rotation [5],
			& manual_spacing_rotation [6],
			& manual_spacing_rotation [7],
			& manual_spacing_rotation [8],
			& manual_spacing_rotation [9],
			& manual_spacing_rotation [10],
			& manual_spacing_rotation [11],
			& manual_spacing_rotation [12],
			& manual_spacing_rotation [13],
			& manual_spacing_rotation [14],
			& manual_spacing_rotation [15]
			);
		if (count != (7+6+16) || !manual_spacing_bottom) {
			warning_str_int ("Lower jaw adjustment data invalid.", count);
		} else {
			// Set the manual spacing flag, to indicate that we
			// will have to open the document in manual spacing
			// mode.
			//
			did_manual_spacing = true;

			manual_spacing_bottom->translate_x = values [0];
			manual_spacing_bottom->translate_y = values [1];
			manual_spacing_bottom->translate_z = values [2];
			manual_spacing_bottom->rotate_x = values [3];
			manual_spacing_bottom->rotate_y = values [4];
			manual_spacing_bottom->rotate_z = values [5];
			manual_spacing_bottom->rotate_angle = values [6];
			manual_spacing_bottom->op_index = 2;
			manual_spacing_bottom->operations[0] = Transform::TRANSLATE;
			manual_spacing_bottom->operations[1] = Transform::ROTATE;

#if 0
			// Construct 4x4 rotation about y axis.
			//
			float a = values[6];
			manual_spacing_rotation[0] = cos (a);
			manual_spacing_rotation[2] = -sin (a);
			manual_spacing_rotation[8] = sin (a);
			manual_spacing_rotation[10] = cos (a);
#endif

			widget_relative_rotation->set_float_array_val (manual_spacing_rotation);

			manual_space_callback (0);
			manual_space_callback (1);
			manual_space_callback (2);
			manual_space_callback (3);
		}
	}
}

//---------------------------------------------------------------------------
// Name:	Model::autocenter
// Purpose:	Determines bounding box for the current model and centers it.
//---------------------------------------------------------------------------
void
Model::autocenter ()
{
	minx = miny = minz = 1E6f;
	maxx = maxx = maxz = -1E6f;

	report_bounds ();
#if 0
	printf ("BOUNDS: %g %g, %g %g, %g %g\n",
		minx,maxx,miny,maxy,minz,maxz);
#endif
	float center_x = (maxx + minx) / 2.0f;
	float center_y = (maxy + miny) / 2.0f;
	float center_z = (maxz + minz) / 2.0f;
#if 0
	printf ("CENTER: %g %g %g\n",
		center_x, center_y, center_z);
#endif

	translate_all (-center_x, -center_y, -center_z);

	// Calculate post-translation min and max values.
	// These are needed for the cross-section views.
	//
	minx -= center_x;
	maxx -= center_x;	
	miny -= center_y;
	maxy -= center_y;
	minz -= center_z;
	maxz -= center_z;
}

//---------------------------------------------------------------------------
// Name:	glui_print_callback
// Purpose:	Callback for printing via PDF.
//---------------------------------------------------------------------------
void 
glui_print_callback (const int control)
{
	char home_path[MAX_PATH];
	memset(home_path, 0, sizeof(char)*MAX_PATH);
	
	bool gotPath = get_temp_print_path(home_path, sizeof(char)*MAX_PATH);
	if (!gotPath)
	{
		warning ("Cannot locate user's home directory.");
		return;
	}

	// Cycle through maxilla01.pdf, maxilla02.pdf, ..., maxilla50.pdf until we find 
	// a free temporary filename we can write to. This is done because Adobe locks
	// any file it has open, so if a temp print file is already open, we can't 
	// write to the same filename.
	int printNumber = 1;
	char filename[30];
	memset(filename, 0, 30);

	while (printNumber <= MAX_TEMP_PDF_FILES)
	{			
		get_temp_print_filename(filename, 30, printNumber);
		strcpy (op_path, home_path);
		strcat (op_path, filename);		

		// Check if file can be opened for writing.
		FILE * pFile = NULL;
		pFile = fopen (op_path, "wb");
		if (pFile != NULL)
		{
			// Success. Close file, exit loop, and use this filename.
			fclose (pFile);
			break;
		}
		
		printNumber++;
	}

	if (printNumber > MAX_TEMP_PDF_FILES)
	{
		warning("All available temporary PDF files used for printing are open in another program. Please close some PDFs and try printing again.");
		op_path[0] = 0;
		return;
	}

	ops_pause = true;

	glutSetWindow (main_window);
	saved_window_width = glutGet(GLUT_WINDOW_WIDTH);
	saved_window_height = glutGet(GLUT_WINDOW_HEIGHT);

	glutReshapeWindow(1200, 712);

	if (doing_multiview) {
		ops_add (OP_PRINTING_BEGIN, false);
		ops_add (OP_MULTIVIEW_PRINTING_1, false);
		ops_add (OP_MULTIVIEW_PRINTING_2, false);
		ops_add (OP_MULTIVIEW_PRINTING_3, false);
		ops_add (OP_MULTIVIEW_PRINTING_4, false);
		ops_add (OP_MULTIVIEW_PRINTING_5, false);
		ops_add (OP_MULTIVIEW_PRINTING_6, false);
		ops_add (OP_PRINTING_END, false);
	} else {
		ops_add (OP_PRINTING_BEGIN, false);
		ops_add (OP_SINGLEVIEW_PRINTING, false);
		ops_add (OP_PRINTING_END, false);
	}

	ops_add (OP_INVOKE_ACROBAT, false);

	if (doing_multiview) {
		int i;
		for (i = 0; i < N_SUBWINDOWS; i++) {
			glutSetWindow (subwindows [i]);
			glutHideWindow ();
		}
	} 

	op_t0 = millisecond_time ();
	op_duration = 500;
	ops_pause = false;
}

char save_stl_fileName[MAX_PATH];
void glui_stl_callback(const int control)
{

	if(!model)
		return;

	ops_pause = true;

	op_path [0] = 0;

	ops_add (OP_INVOKE_STLEXPORT, false);
	
	memset(save_stl_fileName,0,MAX_PATH);	
	strcpy(save_stl_fileName, model->inputfile->path);
	PathRemoveExtension(save_stl_fileName);


#ifndef __APPLE__
	CreateThread (0, 0,(LPTHREAD_START_ROUTINE) save_stlfile_thread, save_stl_fileName, 0, 0);
#else
	save_file_thread (NULL);
#endif

}

void invoke_stlexport()
{

	if(!model)
		return;

	IndexedFaceSet *ifs1;
	IndexedFaceSet *ifs2;
	find_maxilla_mandible (ifs1, ifs2);

	if (!ifs1 || !ifs2) {
		warning ("DST data is not organized as expected, cannot offer Open view.");
		return;
	}

	Group* g = model->top_and_bottom_node;
	Replicated *r1;
	Replicated *r2;
	Transform *t1;
	Transform *t2;
	Node* top = NULL;
	Node* bottom = NULL;

	if (!g)
		return;

	r1 = (Replicated*) g->children;
	r2 = (Replicated*) g->children->next;

	if (strncmp (r1->type, "Replic", 6) ||
		strncmp (r2->type, "Replic", 6)) {
			return;
	}
	
	//------------------------------------------------------------
	// Determine which is the mandible by looking for name 
	// "BottomSet".
	//
	t1 = (Transform*) r1->original;
	t2 = (Transform*) r2->original;			
	if (strncmp (t1->type, "Transf", 6) ||
		strncmp (t2->type, "Transf", 6)) {
			warning ("DST data is not organized as expected, cannot offer Open view.");
			return;
	}
	if (!strncmp ("BottomSet", t2->name, 9)) {
		// As expected.
		top = r1;
		bottom = r2;
	} else {
		Transform *tmp = t1;
		t1 = t2;
		t2 = tmp;

		top = r2;
		bottom = r1;
	}



	char path1[MAX_PATH];
	char path2[MAX_PATH];
	memset(path1, 0, sizeof(char)*MAX_PATH);
	memset(path2, 0, sizeof(char)*MAX_PATH);
	
	if(stricmp(op_path,".stl") >= 0) 
	{
		strncpy(path1, op_path, strlen(op_path)-4);
		strcat(path1, " upper.stl");

		strncpy(path2, op_path, strlen(op_path)-4);
		strcat(path2, " lower.stl");
	}
	else
	{
		strcpy(path1, op_path);
		strcat(path1, " upper.stl");
		strcpy(path2, op_path);
		strcat(path2, " lower.stl");
			
	}

	
	

	STLRenderContext upperContext;
	upperContext.open(path1, ifs1->n_triangles);
	top->express(false, &upperContext);
	upperContext.close();

	STLRenderContext lowerContext;
	lowerContext.open(path2, ifs2->n_triangles);	
	bottom->express(false, &lowerContext);	
	lowerContext.close();

	::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	PathRemoveFileSpec(op_path);

	ShellExecute(NULL, "open", op_path, NULL, NULL, SW_SHOWDEFAULT);

}

void glui_jpg_callback(const int control)
{
	ops_pause = true;

	glutSetWindow (main_window);
	saved_window_width = glutGet(GLUT_WINDOW_WIDTH);
	saved_window_height = glutGet(GLUT_WINDOW_HEIGHT);

	glutReshapeWindow(1200, 712);


	op_path [0] = 0;

	if (doing_multiview) {
		ops_add (OP_PRINTING_BEGIN, false);
		ops_add (OP_MULTIVIEW_PRINTING_1, false);
		ops_add (OP_MULTIVIEW_PRINTING_2, false);
		ops_add (OP_MULTIVIEW_PRINTING_3, false);
		ops_add (OP_MULTIVIEW_PRINTING_4, false);
		ops_add (OP_MULTIVIEW_PRINTING_5, false);
		ops_add (OP_MULTIVIEW_PRINTING_6, false);
		ops_add (OP_PRINTING_END, false);
	} else {
		ops_add (OP_PRINTING_BEGIN, false);
		ops_add (OP_SINGLEVIEW_PRINTING, false);
		ops_add (OP_PRINTING_END, false);
	}

	ops_add (OP_INVOKE_PDF2JPG, false);

#ifndef __APPLE__
	CreateThread (0, 0,(LPTHREAD_START_ROUTINE) save_file_thread, 0, 0, 0);
#else
	save_file_thread (NULL);
#endif
}

//---------------------------------------------------------------------------
// Name:	glui_pdf_callback
// Purpose:	Callback for saving to PDF.
//---------------------------------------------------------------------------
void 
glui_pdf_callback (const int control)
{
	ops_pause = true;

	glutSetWindow (main_window);
	saved_window_width = glutGet(GLUT_WINDOW_WIDTH);
	saved_window_height = glutGet(GLUT_WINDOW_HEIGHT);

	glutReshapeWindow(1200, 712);

	op_path [0] = 0;

	if (doing_multiview) {
		ops_add (OP_PRINTING_BEGIN, false);
		ops_add (OP_MULTIVIEW_PRINTING_1, false);
		ops_add (OP_MULTIVIEW_PRINTING_2, false);
		ops_add (OP_MULTIVIEW_PRINTING_3, false);
		ops_add (OP_MULTIVIEW_PRINTING_4, false);
		ops_add (OP_MULTIVIEW_PRINTING_5, false);
		ops_add (OP_MULTIVIEW_PRINTING_6, false);
		ops_add (OP_PRINTING_END, false);
	} else {
		ops_add (OP_PRINTING_BEGIN, false);
		ops_add (OP_SINGLEVIEW_PRINTING, false);
		ops_add (OP_PRINTING_END, false);
	}

#ifndef __APPLE__
	CreateThread (0, 0,(LPTHREAD_START_ROUTINE) save_file_thread, 0, 0, 0);
#else
	save_file_thread (NULL);
#endif
}

//---------------------------------------------------------------------------
// Name:	glui_save_as_callback
// Purpose:	Callback for saving to a non-compressed DST file.
//---------------------------------------------------------------------------
void 
glui_save_as_callback (const int control)
{
	ops_pause = true;	// Prevent execution of OP_SAVE_AS.

	//----------------------------------------
	// A model file that has never been saved
	// will need to have the VRML structures 
	// added.
	//
	will_need_to_add_position_structures = false;
	Node *position_node = model->nodes->find_by_name ("node_position");
	if (!position_node)
		will_need_to_add_position_structures = true;

	op_path [0] = 0;

	ops_add (OP_SAVE_AS, false);

#ifndef __APPLE__
	CreateThread (0, 0,(LPTHREAD_START_ROUTINE) save_file_thread, 0, 0, 0);
#else
	save_file_thread (NULL);
#endif
}

//---------------------------------------------------------------------------
// Name:	color_chooser
// Purpose:	Provides the user with a color chooser popup.
//---------------------------------------------------------------------------
static bool doing_foreground = false;
static RGB chooser_incoming;
void*
color_chooser (void *foo)
{
#ifdef WIN32
	CHOOSECOLOR cc;
	static COLORREF custom[16];

	memset (custom, 0, sizeof (COLORREF)*16);
	custom[0] = 0xff0000;   // blue
	custom[1] = 0xff00;     // green
	custom[2] = 0xff;       // red
	custom[3] = 0xff00ff;   // purple
	custom[4] = 0xffff;     // yellow
	custom[5] = 0xffff00;   // cyan
	custom[6] = 0xffffff;   // white

	memset (&cc, 0, sizeof (cc));
	cc.lStructSize = sizeof (cc);
	cc.lpCustColors = (LPDWORD) custom;
	cc.rgbResult = chooser_incoming;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColor (&cc)) {
		if (doing_foreground)
			floats_from_rgb (cc.rgbResult, user_fg);
		else
			floats_from_rgb (cc.rgbResult, user_bg);

		ops_add (OP_COLORS, doing_foreground);
	} 

#else
	// XX
#endif
	return NULL;
}

//---------------------------------------------------------------------------
// Name:	glui_fg_color_chooser_callback
// Purpose:	Callback for the foreground color chooser button.
//---------------------------------------------------------------------------
void 
glui_fg_color_chooser_callback (const int control)
{
	doing_foreground = true;
	chooser_incoming = floats_to_rgb (user_fg); 
#ifndef __APPLE__
	CreateThread (0,0,(LPTHREAD_START_ROUTINE) color_chooser, 0,0,0);
#else
	macosx_color_chooser ();
#endif
}

//---------------------------------------------------------------------------
// Name:	glui_bg_color_chooser_callback
// Purpose:	Callback for the background color chooser button.
//---------------------------------------------------------------------------
void 
glui_bg_color_chooser_callback (const int control)
{
	doing_foreground = false;
	chooser_incoming = floats_to_rgb (user_bg); 
#ifndef __APPLE__
	CreateThread (0,0,(LPTHREAD_START_ROUTINE) color_chooser, 0,0,0);
#else
	macosx_color_chooser ();
#endif
}

//---------------------------------------------------------------------------
// Name:	glui_exit_callback
// Purpose:	Callback for the Exit button.
//---------------------------------------------------------------------------
void 
glui_exit_callback (const int control)
{	
	save_user_settings ();
#ifdef WIN32
	myexit (EXIT_SUCCESS);
#else 
	myexit (0);
#endif
}


//---------------------------------------------------------------------------
// Name:	glui_doctor_callback
// Purpose:	Callback for the "Set Practice Name" button.
//---------------------------------------------------------------------------
void 
glui_doctor_callback (const int control)
{	
	const char *doc = widget_doctor_name->get_text ();
	strncpy (doctor_name, doc, PATH_MAX);
	store_doctor_name ();
}

//---------------------------------------------------------------------------
// Name:	handle_idle
// Purpose:	Idle function.
//---------------------------------------------------------------------------
void 
handle_idle ()
{
	if (ops_pause)
		return;

	long t = millisecond_time ();

	int op = ops_get ();
	if (op && (t - op_t0) >= op_duration) 
	{
		switch (op) 
		{
		case OP_PRINTING_BEGIN:
			printing_begin ();
			ops_next ();
			break;

		case OP_PRINTING_END:
			printing_end ();
			ops_next ();
			break;

		case OP_SINGLEVIEW_PRINTING:
			printing_core (0);
			ops_next ();
			break;

		case OP_MULTIVIEW_PRINTING_1:
			printing_core (0);
			ops_next ();
			break;

		case OP_MULTIVIEW_PRINTING_2:
			printing_core (1);
			ops_next ();
			break;

		case OP_MULTIVIEW_PRINTING_3:
			printing_core (2);
			ops_next ();
			break;

		case OP_MULTIVIEW_PRINTING_4:
			printing_core (3);
			ops_next ();
			break;

		case OP_MULTIVIEW_PRINTING_5:
			printing_core (4);
			ops_next ();
			break;

		case OP_MULTIVIEW_PRINTING_6:
			printing_core (5);
			ops_next ();
			break;

		case OP_TEST:
#ifdef WIN32
			MessageBoxA (0, "Testing!", 0,0);
#endif
			ops_next ();
			break;

		case OP_INVOKE_ACROBAT:
			invoke_acrobat ();
			ops_next ();
			break;

		case OP_INVOKE_PDF2JPG:
			invoke_pdf2jpg ();
			ops_next ();
			break;
		case OP_INVOKE_STLEXPORT:
			{
				int len = strlen (op_path);

				if (len < 4 || (stricmp (op_path+len-4, ".stl")))
					strcat (op_path, ".stl");
			
				invoke_stlexport();
				ops_next ();
			break;
			}
		case OP_SAVE_AS: {
			int len = strlen (op_path);
#ifndef __APPLE__
			if (len < 4 || (stricmp (op_path+len-4, ".dst")))
				strcat (op_path, ".dst");
#else
			if (len < 4 || (strcasecmp (op_path+len-4, ".dst")))
				strcat (op_path, ".dst");
#endif
			gzFile f = gzopen (op_path, "wb");
			serialization_indentation_level = 0;
			model->serialize (f);
			gzclose (f);
			gui_set_status ("Saved model to DST file.");
			ops_next ();
		  }
			break;

		case OP_EXIT:
			myexit (0);
			break;

		case OP_OPEN: {
			ops_next ();

			op_param = false;

			gui_set_status ("Loading file...");

			InputFile *file = new InputFile (op_path);

			op_path[0] = 0;

			// If InputFile constructor didn't like the extension
			// it will flag itself as invalid.
			//
			if (!file->valid) {
				warning ("Unsupported file extension");
				delete file;
				return;
			}

			if (input_file)
				close_file ();

			if (model) {
				delete model;
				model = NULL;
			}

			load_file (file);

			// If the parser indicates that manual spacing info
			// was read successfully from the file, then let's
			// switch now to manual spacing mode.
			//
			if (did_manual_spacing) {
				if (widget_multiview)
					widget_multiview->disable ();
				widget_cutaway->disable ();
				widget_mandible->disable ();
				widget_maxilla->disable ();
				widget_mandible->set_int_val (1);
				widget_mandible->set_int_val (1);
				widget_mandible->disable ();
				widget_maxilla->disable ();
				widget_manual_spacing->set_int_val (true);
				glui_checkbox_callback (0);
				// XX here
			}

			glutPostRedisplay ();
			return;
		 }

		case OP_COLORS:
			save_user_settings ();
			draw_scene ();
			draw_scene ();
			
			ops_next ();

			op_param = false;
			return;
		}
	}
#ifdef WIN32
	Sleep (1);
#else
	usleep (1000);
#endif
}

void
set_secondary_value (int which, float value)
{
	if (which < 0 || which >= N_MEASUREMENT_WIDGETS)
		return; // XX
	if (value < 0.f)
		value = 0.f; // just in case

	secondary_measurements [which] = value;

	char str [100];
	int i = (int) floor(value);
	int j = (int) floor(value * 10.f);
	j %= 10;
	sprintf (str, "%d.%01d", i, j);
	secondary_measurement_widgets[which]->set_text (str);

	//------------------------------
	// Get sum of all nonzero values.
	//
	// First get average of nonzero lower measurements.
	float total = 0.f;
	float sum_upper, sum_lower;
	int n_nonzero = 0;
	int n = N_MEASUREMENT_WIDGETS / 2;
	for (i = 0; i < n; i++) {
		if (secondary_measurements [i] > 0.f) {
			n_nonzero++;
			total += secondary_measurements [i];
		}
	}
	sum_upper = total;
	
	// Get average of nonzero lower measurements.
	n_nonzero = 0;
	total = 0.f;
	n *= 2;
	for (; i < n; i++) {
		if (secondary_measurements [i] > 0.f) {
			n_nonzero++;
			total += secondary_measurements [i];
		}
	}
	sum_lower = total;

	// Get ratio only if both averages are nonzero.
	float ratio = 0.f;
	if (sum_lower > 0.f && sum_upper > 0.f)
		ratio = sum_lower / sum_upper;

	// Write results to the widgets.
	char str1[100];
	char str2[100];
	char str3[100];
	sprintf (str1, "Upper sum : %4.2g mm", sum_upper);
	sprintf (str2, "Lower sum: %4.2g mm", sum_lower);
	sprintf (str3, "Bolton Ratio: %4.2g", ratio);

	widget_maxillary_sum->set_text (str1);
	widget_mandibular_sum->set_text (str2);
	widget_ratio->set_text (str3);
}

//---------------------------------------------------------------------------
// Name:	Callbacks for Bolton measurement dots.
//---------------------------------------------------------------------------
void
secondary_dot_start (int which)
{
	red_dot_remove_all ();

	if (!doing_measuring) {
		widget_button_bolton_start[which]->set_name ("Done");

		// Ideally one would set the background color of the
		// text field widget to something that stands out here....
		// but GLUI doesn't allow that.

		doing_measuring = rotation_locked = true;
		secondary_current_measurement = which;
		set_secondary_value (secondary_current_measurement, 0.f);
	} else {
		widget_button_bolton_start[which]->set_name ("Start");
		doing_measuring = rotation_locked = false;
	}
}

void
secondary_dot_clear (int which)
{
	red_dot_remove_all ();
	set_secondary_value (secondary_current_measurement, 0.f);

	widget_button_bolton_start[which]->set_name ("Start");
	doing_measuring = rotation_locked = false;
}

void
cb_bolton_printing (int foo)
{
	char fullpath [PATH_MAX];
#ifdef WIN32
	char *homedir = getenv ("HOMEPATH");
	strcpy (fullpath, "c:");
	strcat (fullpath, homedir);
	strcat (fullpath, "\\print.htm");
#endif

	int i;

	//------------------------------------------------------
	// Same code as above...
	//------------------------------
	// Get sum of all nonzero values.
	//
	// First get average of nonzero lower measurements.
	float total = 0.f;
	float sum_upper, sum_lower;
	int n_nonzero = 0;
	int n = N_MEASUREMENT_WIDGETS / 2;
	for (i = 0; i < n; i++) {
		if (secondary_measurements [i] > 0.f) {
			n_nonzero++;
			total += secondary_measurements [i];
		}
	}
	sum_upper = total;
	
	// Get average of nonzero lower measurements.
	n_nonzero = 0;
	total = 0.f;
	n *= 2;
	for (; i < n; i++) {
		if (secondary_measurements [i] > 0.f) {
			n_nonzero++;
			total += secondary_measurements [i];
		}
	}
	sum_lower = total;

	// Get ratio only if both averages are nonzero.
	float ratio = 0.f;
	if (sum_lower > 0.f && sum_upper > 0.f)
		ratio = sum_lower / sum_upper;
	//------------------------------------------------------

	if (!model) {
		warning ("No model was loaded.");
		return;
	}

	FILE *f = fopen (fullpath, "wb");
	fprintf (f, "<html><head><title>Maxilla Bolton Measurements</title>");
	fprintf (f, "</head><body>");

	fprintf (f, "<h1>Maxilla Bolton Measurements</h1>");

	if (model->patient_firstname || model->patient_lastname) {
		fprintf (f, "<h2>Patient %s %s</h2>", 
			model->patient_firstname ? model->patient_firstname : "",
			model->patient_lastname ? model->patient_lastname : "");
	} else {
		fprintf (f, "<h2>No patient name in file.</h2>");
	}

	fprintf (f, "Upper sum: %4.2g mm <br>", sum_upper);
	fprintf (f, "Lower sum: %4.2g mm <br>", sum_lower);
	fprintf (f, "Bolton Ratio: %4.2g <br>", ratio);

	fprintf (f, "</body></html>");
	fclose (f);

	char cmd [PATH_MAX];
	sprintf (cmd, "explorer \"%s\"", fullpath);
	system (cmd);
}

void 
cb_switch_windows (int from)
{
	doing_measuring = false;
	widget_measuring->set_int_val (0);

	red_dot_remove_all ();

	if (!from) {
		// Switch back to main window.
		//
		showing_bolton = false;

		glutSetWindow (secondary_window);
		glutHideWindow ();
		
		glutSetWindow (main_window);
		glutShowWindow ();

		occlusal_callback (0);
		widget_multiview->set_int_val (0);
		multiview_callback (0);

		pan_live_vars[0] = 0;
		pan_live_vars[1] = 0;
		pan_live_vars[2] = 0;
	} else {
		// Switch to secondary window.
		//
		if (doing_multiview) {
			widget_multiview->set_int_val (true);
			multiview_callback (0);
			
			widget_occlusal->enable ();
			doing_multiview = false;
		}

		using_measuring_lines = false;

		occlusal_callback (0);

		widget_multiview->enable ();
		showing_occlusal = false;
		showing_bolton = true;
		occlusal_callback (0);
		doing_measuring = false;
		
		glutSetWindow (main_window);
		glutHideWindow ();
		
		glutSetWindow (secondary_window);
		glutShowWindow ();
	}

	// Fix the aspect ratio.
	handle_resize (1,1);
}

void
clear_secondary_measurements ()
{
	int i;
	for (i = 0; i < N_MEASUREMENT_WIDGETS; i++) {
		secondary_measurements [i] = 0.f;
		secondary_measurement_widgets[i]->set_text ("0.0");
	}
}

//---------------------------------------------------------------------------
// Name:	setup_secondary_window_widgets
// Purpose:	Setup GLUI and create side menu widgets.
//---------------------------------------------------------------------------
void 
setup_secondary_window_widgets ()
{
	init_renderer ();

	//------------------------------
	// Set up GLUT callbacks.
	//
	GLUI_Master.set_glutDisplayFunc (draw_scene); 
	GLUI_Master.set_glutKeyboardFunc (handle_keypress);
	GLUI_Master.set_glutSpecialFunc (handle_special);
	GLUI_Master.set_glutReshapeFunc (handle_resize);
	GLUI_Master.set_glutIdleFunc (handle_idle);
	GLUI_Master.set_glutMouseFunc (handle_mouse);
	glutMotionFunc (handle_motion);

	//------------------------------
	// Set up 2nd window widgets...
	//------------------------------
	
	glui2_left = GLUI_Master.create_glui_subwindow (secondary_window, GLUI_SUBWINDOW_RIGHT); 
	glui2_left->set_main_gfx_window (secondary_window);

	//----------------------------------------
	// Create the set of 30 or so rows.
	//
	GLUI_Panel *p = glui2_left->add_panel ("", 0);
	p->set_w (80);
	int i;
	for (i = 0; i < N_MEASUREMENT_WIDGETS; i++) {
		GLUI_EditText *t;
		t = secondary_measurement_widgets[i] = new GLUI_EditText (p,
			"0.0   ");
		t->set_name (measurement_names [i]);
		t->set_w (80);
		t->set_h (20);  
	}
#if 0
	for (i = 0; i < N_MEASUREMENT_WIDGETS; i++) {
		GLUI_StaticText *t = new GLUI_StaticText (p, "");
		t->set_h (20);  
		t->set_w (20); 
	}
#endif
	glui2_left->add_column_to_panel (p, false);	
	for (i = 0; i < N_MEASUREMENT_WIDGETS; i++) {
		GLUI_Button *b =
			widget_button_bolton_start[i] = new GLUI_Button (p, "Start", i,
				(GLUI_Update_CB) secondary_dot_start);
		b->set_w (40); 
		b->set_h (20);
	}
	glui2_left->add_column_to_panel (p, false);	
	for (i = 0; i < N_MEASUREMENT_WIDGETS; i++) {
		GLUI_Button *b = new GLUI_Button (p, "Clear", i,
			(GLUI_Update_CB) secondary_dot_clear);
		b->set_w (40); 
		b->set_h (20);  
	}

	clear_secondary_measurements ();

	//--------------------
	// Put others on right.
	//
	glui2_right = GLUI_Master.create_glui_subwindow (secondary_window, GLUI_SUBWINDOW_RIGHT);
	glui2_right->set_main_gfx_window (secondary_window);

	GLUI_StaticText *title =
		new GLUI_StaticText (glui2_right, "Bolton Analysis");
	title->set_font (GLUT_BITMAP_HELVETICA_18);

	//new GLUI_StaticText (glui2_right, "");

	new GLUI_Button (glui2_right, "Return", 0, 
		(GLUI_Update_CB) cb_switch_windows);

	GLUI_Panel *image_panel = glui2_right->add_panel ("", 0);
	bolton_widget_zoom = new GLUI_Translation (image_panel, "Zoom",
		GLUI_TRANSLATION_Z, &zoom_live_var, -1, glui_zoom_callback);
	bolton_widget_zoom->set_speed (-.0001);

	glui2_right->add_column_to_panel (image_panel, false);
	bolton_widget_translation = new GLUI_Translation (image_panel, "Pan", 
		GLUI_TRANSLATION_XY, pan_live_vars, -1, glui_translate_callback);
	bolton_widget_translation->set_speed (-.0001);

	new GLUI_StaticText (glui2_right, "");

	widget_maxillary_sum = 
		new GLUI_StaticText (glui2_right, "Maxillary Sum: 0.0 mm");
	widget_mandibular_sum = 
		new GLUI_StaticText (glui2_right, "Mandibular Sum: 0.0 mm");
	widget_ratio = 
		new GLUI_StaticText (glui2_right, "Bolton Ratio: N/A");

	new GLUI_StaticText (glui2_right, "");
	new GLUI_Button (glui2_right, "Print", 0, 
		(GLUI_Update_CB) cb_bolton_printing);

#if 0
	new GLUI_StaticText (glui2_right, "");

	button_lock_rotation = new GLUI_Button (glui2_right, "Lock Rotation", 
		0, (GLUI_Update_CB) cb_bolton_lock);
#endif
}

//---------------------------------------------------------------------------
// Name:	setup_main_window_widgets
// Purpose:	Setup GLUI and create side menu widgets.
//---------------------------------------------------------------------------
void 
setup_main_window_widgets ()
{
	init_renderer ();

	//------------------------------
	// Set up GLUT callbacks.
	//
	GLUI_Master.set_glutDisplayFunc (draw_scene); 
	GLUI_Master.set_glutKeyboardFunc (handle_keypress);
	GLUI_Master.set_glutSpecialFunc (handle_special);
	GLUI_Master.set_glutReshapeFunc (handle_resize);
	GLUI_Master.set_glutIdleFunc (handle_idle);
	GLUI_Master.set_glutMouseFunc (handle_mouse);
	glutMotionFunc (handle_motion);

	//------------------------------
	// Set up GLUI widgets...
	//
	glui_right = GLUI_Master.create_glui_subwindow (main_window, GLUI_SUBWINDOW_RIGHT);
	glui_right->set_main_gfx_window (main_window);

#ifndef ORTHOCAST
	new GLUI_StaticText (glui_right, "Maxilla, release" PROGRAM_VERSION);
	new GLUI_StaticText (glui_right, "Copyright (C) 2008-2013");
	new GLUI_StaticText (glui_right, "by Zack Smith & Ortho Cast Inc.");

	new GLUI_StaticText (glui_right, "Provided AS-IS,");
	new GLUI_StaticText (glui_right, "use at your own risk.");
	new GLUI_StaticText (glui_right, "See included file");
	new GLUI_StaticText (glui_right, "COPYING for license.");
	
	widget_filename = new GLUI_StaticText (glui_right, "File: -");
	widget_title = new GLUI_StaticText (glui_right, "Title: -");
#else
	GLUI_StaticText *te0;
	te0 = new GLUI_StaticText (glui_right, "Patient Information");
	te0->set_font (GLUT_BITMAP_HELVETICA_18);

	//----------------------------------------------
	// Set up nicely aligned columns of patient data.
	//
	GLUI_Panel *pt_panel = glui_right->add_panel ("", 0);
	GLUI_StaticText *tt;
	tt = new GLUI_StaticText (pt_panel, "Last name");
	tt->set_h (20);  tt->set_w (20); 	
	tt = new GLUI_StaticText (pt_panel, "First name");
	tt->set_h (20);  tt->set_w (20); 
	tt = new GLUI_StaticText (pt_panel, "Birth date");
	tt->set_h (20);  tt->set_w (20); 
	tt = new GLUI_StaticText (pt_panel, "Case #");
	tt->set_h (20);  tt->set_w (20); 
	tt = new GLUI_StaticText (pt_panel, "Case date");
	tt->set_h (20);  tt->set_w (20); 
	tt = new GLUI_StaticText (pt_panel, "Control #");
	tt->set_h (20);  tt->set_w (20); 
	glui_right->add_column_to_panel (pt_panel, false);

	widget_patient_last_name = new GLUI_EditText (pt_panel, "");
	widget_patient_last_name->set_w (170);
	widget_patient_last_name->set_h (20);

	widget_patient_first_name = new GLUI_EditText (pt_panel, "");
	widget_patient_first_name->set_w (170);
	widget_patient_first_name->set_h (20);

	widget_patient_birth_date = new GLUI_EditText (pt_panel, "");
	widget_patient_birth_date->set_w (170);
	widget_patient_birth_date->set_h (20);

	widget_case_number = new GLUI_EditText (pt_panel, "");
	widget_case_number->set_w (170);
	widget_case_number->set_h (20);

	widget_case_date = new GLUI_EditText (pt_panel, "");
	widget_case_date->set_w (170);
	widget_case_date->set_h (20);

	widget_control_number = new GLUI_EditText (pt_panel, "");
	widget_control_number->set_w (170);
	widget_control_number->set_h (20);

#if 0
	// Results in poor alignment:
	widget_patient_last_name = glui_right->add_edittext_to_panel (pt_panel,
		"Last name");
	widget_patient_first_name = glui_right->add_edittext_to_panel (pt_panel,
		"First name");
	widget_patient_birth_date = glui_right->add_edittext_to_panel (pt_panel,
		"Birth date");
	widget_case_number = glui_right->add_edittext_to_panel (pt_panel,
		"Case #");
	widget_case_date = glui_right->add_edittext_to_panel (pt_panel,
		"Case date");
	widget_control_number = glui_right->add_edittext_to_panel (pt_panel,
		"Control #");
	widget_case_date->set_w (170);
	widget_case_number->set_w (170);
	widget_control_number->set_w (170);
	widget_patient_birth_date->set_w (170);
	widget_patient_first_name->set_w (170);
	widget_patient_last_name->set_w (170);
#endif

	new GLUI_Separator (glui_right);

	widget_measuring = new GLUI_Checkbox (glui_right, "Measure point to point",
		0, -1, glui_measuring_callback);
	widget_measuring->set_font (GLUT_BITMAP_HELVETICA_18);
	GLUI_Panel *dot_panel = glui_right->add_panel ("", 0);
 	new GLUI_Button (dot_panel, "Remove last dot", 0,
			 (GLUI_Update_CB)red_dot_remove_last);
	glui_right->add_column_to_panel (dot_panel, false);
 	new GLUI_Button (dot_panel, "Remove all dots", 0,
			 (GLUI_Update_CB)red_dot_remove_all);

	new GLUI_Separator (glui_right);

	widget_manual_spacing= new GLUI_Checkbox (glui_right, "Adjust lower jaw", 0, -1,
		glui_checkbox_callback);
	widget_manual_spacing->set_font (GLUT_BITMAP_HELVETICA_18);
	widget_manual_spacing->set_int_val (doing_manual_spacing);
	
	GLUI_Panel *inner_panel = glui_right->add_panel ("", 0);
	widget_relative_translation_x = glui_right->add_translation_to_panel (inner_panel, "Left/Right", 
		GLUI_TRANSLATION_X, &manual_spacing_viewpoints[0], 0, manual_space_callback);
	widget_relative_translation_x->set_speed (.0001);
	glui_right->add_column_to_panel (inner_panel, false);
	widget_relative_translation_y = glui_right->add_translation_to_panel (inner_panel, "Up/Down", 
		GLUI_TRANSLATION_Y, &manual_spacing_viewpoints[1], 1, manual_space_callback);
	widget_relative_translation_y->set_speed (.0001);
	glui_right->add_column_to_panel (inner_panel, false);
	widget_relative_translation_z = glui_right->add_translation_to_panel (inner_panel, "In/Out", 
		GLUI_TRANSLATION_Z, &manual_spacing_viewpoints[2], 2, manual_space_callback);
	widget_relative_translation_z->set_speed (-.0001);
	glui_right->add_column_to_panel (inner_panel, false);
	widget_relative_rotation = glui_right->add_rotation_to_panel (inner_panel, "Rotation",
		manual_spacing_rotation, 3, manual_space_callback);

	new GLUI_Separator (glui_right);

	widget_cutaway = new GLUI_Checkbox (glui_right, "Cutaway", 0, -1, glui_checkbox_callback);
	widget_cutaway->set_font (GLUT_BITMAP_HELVETICA_18);
	GLUI_Panel *cut_away_panel = glui_right->add_panel ("", false);
	GLUI_Button *foo = glui_right->add_button_to_panel (cut_away_panel, "Axis", 1, glui_cut_away_axis_callback);
	foo->set_w (50);

	button_overbite = glui_right->add_button_to_panel (cut_away_panel, "Overbite", 0, glui_line_dir_callback);
	button_overbite->set_w (50);
	
	button_overjet = glui_right->add_button_to_panel (cut_away_panel, "Overjet", 1, glui_line_dir_callback);
	button_overjet->set_w (50);
	
	glui_right->add_column_to_panel (cut_away_panel, false);
 
	widget_cutaway_trans = glui_right->add_translation_to_panel (cut_away_panel, "Location", 1, &cut_away_plane_location, -1, glui_cut_away_position_callback);
	widget_cutaway_trans->set_speed (-.0001);
	
	glui_right->add_column_to_panel (cut_away_panel, false);
	widget_line0 = glui_right->add_translation_to_panel (cut_away_panel, "Line 1", 1, &line1_value, -1, glui_line1_callback);
	widget_line0->set_speed (.0001); // <-- vital
	
	glui_right->add_column_to_panel (cut_away_panel, false);
	widget_line1 = glui_right->add_translation_to_panel (cut_away_panel, "Line 2", 1, &line2_value, -1, glui_line2_callback);
	widget_line1->set_speed (.0001); // <-- vital

	new GLUI_Separator (glui_right);

	GLUI_StaticText *te4 = new GLUI_StaticText (glui_right, "Image");
	te4->set_font (GLUT_BITMAP_HELVETICA_18);

	GLUI_Panel *image_panel = glui_right->add_panel ("", 0);
	widget_zoom = new GLUI_Translation (image_panel, "Zoom",
		GLUI_TRANSLATION_Z, &zoom_live_var, -1, glui_zoom_callback);
	widget_zoom->set_speed (-.001);

	glui_right->add_column_to_panel (image_panel, false);
	widget_translation = new GLUI_Translation (image_panel, "Pan", 
		GLUI_TRANSLATION_XY, pan_live_vars, -1, glui_translate_callback);
	widget_translation->set_speed (-.0001);

	new GLUI_Separator (glui_right);

#ifndef __APPLE__
	//------------------------------
	// The Mac OS/X port does not 
	// provide a color chooser popup.
	//
	GLUI_StaticText *st1 = new GLUI_StaticText (glui_right, "Colors");
	st1->set_font (GLUT_BITMAP_HELVETICA_18);
	widget_colors = new GLUI_Checkbox (glui_right, "Use custom colors", 
		NULL, -1, glui_user_colors_callback);
	GLUI_Panel *color_panel = glui_right->add_panel ("", 0);
	widget_colors->set_int_val (using_custom_colors);
	new GLUI_Button (color_panel, "Model color", 0, glui_fg_color_chooser_callback);
	glui_right->add_column_to_panel (color_panel, false);
	new GLUI_Button (color_panel, "Background color", 0, glui_bg_color_chooser_callback);

//	new GLUI_Separator (glui_right);
#endif
#endif

	//----------------------------------------------
	// Bottom row
	//
	glui_bottom = GLUI_Master.create_glui_subwindow (main_window, 
		GLUI_SUBWINDOW_BOTTOM);
	glui_bottom->set_main_gfx_window (main_window);

	widget_status_line = new GLUI_StaticText (glui_bottom, 
		"Right-click to see file menu...");
	widget_status_line->set_w (250);

	glui_bottom->add_column (true);
	widget_doctor_name = new GLUI_EditText (glui_bottom, "Practice Name:");
	widget_doctor_name->set_w (300);
	widget_doctor_name->set_text (doctor_name);
	glui_bottom->add_column (false);
	GLUI_Button *docbutton = new GLUI_Button (glui_bottom, "Set", 0, (GLUI_Update_CB) glui_doctor_callback);
	docbutton->set_w (30);

	//----------------------------------------------
	// Bottom row
	//
	GLUI *glui_top = GLUI_Master.create_glui_subwindow (main_window, 
		GLUI_SUBWINDOW_TOP);
	glui_top->set_main_gfx_window (main_window);


#ifndef __APPLE__
	widget_multiview = new GLUI_Checkbox (glui_top, "Multiview",
		0, -1, multiview_callback);
	glui_top->add_column (false);
#endif
	widget_occlusal = new GLUI_Button (glui_top, "Occlusal", 0, occlusal_callback);
	widget_occlusal->set_w (53);
	glui_top->add_column (false);
	widget_occlusal_aligned = new GLUI_Button (glui_top, "Occlusal 2", 0, occlusal2_callback);
	widget_occlusal_aligned->set_w (53);
	glui_top->add_column (false);

	widget_anterior = new GLUI_Button (glui_top, "Anterior", VIEW_FRONT, glui_view_callback);
	widget_anterior->set_w (53);
	glui_top->add_column (false);
	widget_lingual = new GLUI_Button (glui_top, "Lingual", VIEW_BACK, glui_view_callback);
	widget_lingual->set_w (53);
	glui_top->add_column (false);
	widget_left = new GLUI_Button (glui_top, "Left", VIEW_LEFT, glui_view_callback);
	widget_left->set_w (48);
	glui_top->add_column (false);
	widget_right = new GLUI_Button (glui_top, "Right", VIEW_RIGHT, glui_view_callback);
	widget_right->set_w (48);
	glui_top->add_column (true);

	GLUI_StaticText* stext = glui_top->add_statictext("Show:");
	//stext->set_alignment(1);
	stext->set_w(32);
	glui_top->add_column (false);

	widget_maxilla = new GLUI_Checkbox (glui_top, "maxilla", 
		0, -1, glui_checkbox_callback);
	widget_maxilla->set_int_val (showing_maxilla);
	glui_top->add_column (false);

	widget_mandible = new GLUI_Checkbox (glui_top, "mandible", 
		0, -1, glui_checkbox_callback);
	widget_mandible->set_int_val (showing_mandible);
	glui_top->add_column (true);

	GLUI_Button *widget_open = new GLUI_Button (glui_top, "Open", 5, glui_open_file_callback);
	widget_open->set_w (40);
	glui_top->add_column (false);

	GLUI_Button *widget_print = new GLUI_Button (glui_top, "Print", 0, glui_print_callback);
	widget_print->set_w (43);
	glui_top->add_column (false);

	GLUI_Button *widget_pdf = new GLUI_Button (glui_top, "Save PDF", 0, glui_pdf_callback);
	widget_pdf->set_w (43);
	glui_top->add_column (false);

	GLUI_Button *widget_jpg = new GLUI_Button (glui_top, "Save JPG", 0, glui_jpg_callback);
	widget_jpg->set_w (43);
	glui_top->add_column (false);

	GLUI_Button *widget_stl = new GLUI_Button (glui_top, "Save STL", 0, glui_stl_callback);
	widget_stl->set_w (43);
	glui_top->add_column (false);

#ifndef __APPLE__
	GLUI_Button *widget_2nd = new GLUI_Button (glui_top, "Bolton", 1, cb_switch_windows);
	widget_2nd->set_w (43);
	//widget_2nd->disable ();
	glui_top->add_column (false);
#endif

	GLUI_Button *widget_save = new GLUI_Button (glui_top, "Save As", 0, glui_save_as_callback);
	widget_save->set_w (45);
	glui_top->add_column (true);

#if 0
 	GLUI_Button *widget_help = new GLUI_Button (glui_top, "Help", true, (GLUI_Update_CB) popup_help);
	widget_help->set_w (42);
	glui_top->add_column (false);
#endif

	glui_top->add_column (false);
	GLUI_Button *widget_download = new GLUI_Button (glui_top, "Download Models", false, (GLUI_Update_CB) popup_downloadModels);		
	widget_download->set_w (110);
	//glui_top->add_column (false);

	/*
 	GLUI_Button *widget_about = new GLUI_Button (glui_top, "About", false, (GLUI_Update_CB) popup_about);
	widget_about->set_w (50);
	glui_top->add_column (false);

	GLUI_Button *button_exit =
		new GLUI_Button (glui_top, "Exit", 0, (GLUI_Update_CB) glui_exit_callback);
	button_exit->set_w (40);
	*/

	//----------------------------------------------
	// Subwindows
	//
	int x, y, w, h;
	GLUI_Master.get_viewport_area (&x, &y, &w, &h);

	int subwin_extra = 0;
	int interwindow_space = 10;
	subwindows [0] = glutCreateSubWindow (main_window, 
		0,0,1,1);
	glutDisplayFunc (draw_scene);
	glutReshapeFunc (handle_resize);
	glutKeyboardFunc (handle_keypress);
	glutMouseFunc (handle_mouse);
	glutMotionFunc (handle_motion);
	init_renderer ();
	glutHideWindow ();

	subwindows [1] = glutCreateSubWindow (main_window, 
		0,0,1,1);
	glutDisplayFunc (draw_scene);
	glutReshapeFunc (handle_resize);
	glutKeyboardFunc (handle_keypress);
	glutMouseFunc (handle_mouse);
	glutMotionFunc (handle_motion);
	init_renderer ();
	glutHideWindow ();

	subwindows [2] = glutCreateSubWindow (main_window, 
		0,0,1,1);
	glutDisplayFunc (draw_scene);
	glutReshapeFunc (handle_resize);
	glutKeyboardFunc (handle_keypress);
	glutMouseFunc (handle_mouse);
	glutMotionFunc (handle_motion);
	init_renderer ();
	glutHideWindow ();

	subwindows [3] = glutCreateSubWindow (main_window, 
		0,0,1,1);
	glutDisplayFunc (draw_scene);
	glutReshapeFunc (handle_resize);
	glutKeyboardFunc (handle_keypress);
	glutMouseFunc (handle_mouse);
	glutMotionFunc (handle_motion);
	init_renderer ();
	glutHideWindow ();

#if N_SUBWINDOWS==6
	subwindows [4] = glutCreateSubWindow (main_window, 
		0, 0, 1, 1);
	glutDisplayFunc (draw_scene);
	glutReshapeFunc (handle_resize);
	glutKeyboardFunc (handle_keypress);
	glutMouseFunc (handle_mouse);
	glutMotionFunc (handle_motion);
	init_renderer ();
	glutHideWindow ();

	subwindows [5] = glutCreateSubWindow (main_window, 
		0,0,1,1);
	glutDisplayFunc (draw_scene);
	glutReshapeFunc (handle_resize);
	glutKeyboardFunc (handle_keypress);
	glutMouseFunc (handle_mouse);
	glutMotionFunc (handle_motion);
	init_renderer ();
	glutHideWindow ();
#endif

	for (int i = 0; i < N_SUBWINDOWS; i++) {
		cc_subwindows[i].configuration = SHOW_BOTH;
	}

	cc_main.configuration = SHOW_BOTH;
}

//---------------------------------------------------------------------------
// Name:	main
// Purpose:	Main routine.
//---------------------------------------------------------------------------
#ifdef WIN32
int 
_tmain (const int argc, const _TCHAR* argv[])
#else
int 
main (int argc, char **argv)
#endif
{
	int i;
	InputFile *file = NULL;

	did_manual_spacing = false;

	// Add custom handler to catch app exiting, so we can clean up.
	atexit(atexithandler);

#if defined (WIN32) && !defined (DEBUG)
	//----------------------------------------
	// Hide the Windows console window.
	//
	HWND hWnd = GetConsoleWindow ();
	ShowWindow (hWnd, SW_HIDE);
#endif

	//----------------------------------------
	// Initialize the delayed op subsystem.
	//
	ops_init ();
	op_t0 = 0;
	op_path[0] = 0;

	//----------------------------------------
	// Initialize diagnostics & log file.
	//
	time_program_start = millisecond_time ();
	diag_init ();

	doctor_name [0] = 0;

	using_measuring_lines = false;
	measuring_line_1 = -0.5f;
	measuring_line_2 = 0.5f;

	total_allocated = 0;

	arbitrary_x_translate = 0.f;
	arbitrary_y_translate = 0.f;

	which_cross_section = 0; // not doing cross-section view.

	redrawing_for_selection = false; // not doing selection.
	red_dot_stack = NULL;

	printf ("This is Maxilla, version %s.\n", PROGRAM_RELEASE);
#ifndef ORTHOCAST
	printf ("Copyright (C) 2008-2013 by Zack Smith and Ortho Cast Inc.\n");
#else
	printf ("Copyright (C) 2008-2013 Ortho Cast Inc. and Zack Smith\n");
#endif
	printf ("This program is provided AS-IS and is covered by the GNU Public License v2.\n");
	printf ("Use it at your own risk.\n");
	printf ("See the included file entitled COPYING for more details.\n\n");

	doing_smooth_shading = true;
	doing_orthographic = true;
	using_custom_colors = true;
	doing_multiview = false;
	doing_measuring = false;

	user_shininess = 127;
	user_specularity = 1.0f; 
	user_emissivity = 0.f;
	user_fg[0] = user_fg[1] = user_fg[2] = 1.f;
	user_bg[0] = user_bg[1] = 0.f;
	user_bg[2] = 0.5;

	RGB fg, bg;
	fg = floats_to_rgb (user_fg);
	bg = floats_to_rgb (user_bg);
	if (read_user_parameter32 ("foreground", fg))
		floats_from_rgb (fg, user_fg);
	if (read_user_parameter32 ("background", bg))
		floats_from_rgb (bg, user_bg);

	retrieve_doctor_name ();

	//------------------------------
	// Parse command-line arguments.
	//
	bool next_is_pdf_path = false;
	i = 1;
	while (i < argc) {
		char tmp[PATH_MAX];
#ifdef WIN32
		//------------------------------
		// Copy over argument, which may
		// be wide chars.
		//
		int j=0;
		char ch;
		while (ch = (char) argv[i][j]) {
			tmp[j++] = ch;
			if (j == PATH_MAX-1)
				break;
		}
		tmp[j]=0;
#else
		strncpy (tmp, argv[i], PATH_MAX);
#endif
		if (tmp[0] != '-') {
			//------------------------------
			// Argument is a path.
			//
			if (next_is_pdf_path) {
				//----------------------------------------
				// Set up delayed multiview printing.
				//
				puts ("Automatic printing is scheduled to occur.");
				op_t0 = millisecond_time ();
				
				ops_add (OP_PRINTING_BEGIN, false);
				ops_add (OP_MULTIVIEW_PRINTING_1, false);
				ops_add (OP_MULTIVIEW_PRINTING_2, false);
				ops_add (OP_MULTIVIEW_PRINTING_3, false);
				ops_add (OP_MULTIVIEW_PRINTING_4, false);
				ops_add (OP_MULTIVIEW_PRINTING_5, false);
				ops_add (OP_MULTIVIEW_PRINTING_6, false);
				ops_add (OP_PRINTING_END, false);
				ops_add (OP_EXIT, false);

				op_duration = 2000;
				strcpy (op_path, tmp);
				next_is_pdf_path = false;
			}
			else {
				//----------------------------------------
				// Load the model file.
				//
				if (file) {
					warning ("More than one file specified, using last only.");
				} else {
					file = new InputFile (tmp);
				}
			}
		} 
		else {
			if (!strcmp ("-pdf", tmp)) 
				next_is_pdf_path = true;
			else 
				printf ("Unknown parameter: %s\n", tmp);
		}
		
		i++;
	}

	fflush (stdout);

	//----------------------------------------
	// Initialize Light 0.
	// Non-ortho
	light0.ambient_non_ortho = -0.3f;
	light0.pos_non_ortho[0] = 1.f;
	light0.pos_non_ortho[1] = -100.f;
	light0.pos_non_ortho[2] = -230.f;
	light0.brightness_non_ortho = 1.1f;
	light0.specularity_non_ortho = 1000.0f;
	// Ortho
	light0.ambient_ortho = -.1;
	light0.brightness_ortho = .9;
	light0.specularity_ortho = 1.0f;
	light0.pos_ortho[0] = 0.6f;
	light0.pos_ortho[1] = 0.f;
	light0.pos_ortho[2] = -3.f;
	// Cross-section
	light0.ambient_cross_section = -0.8f;
	light0.pos_cross_section[0] = 3.f; 
	light0.pos_cross_section[1] = -10.f;
	light0.pos_cross_section[2] = -151.f; 
	light0.brightness_cross_section = 1.7f;
	light0.specularity_cross_section = 1.f;
	// Reset
	light0.reset (doing_orthographic ? 'o' : 'n');

	//----------------------------------------
	// Initialize GLUT.
	char *args[] = {"maxilla.exe", NULL}; 
	int num = 1;
	glutInit (&num, args); 

	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize (1360, 712); 
	glutInitWindowPosition (10, 0);

#ifdef ORTHOCAST
	main_window = glutCreateWindow ("Maxilla "PROGRAM_RELEASE" (C)2008-2013 by Ortho Cast Inc, Zack Smith");
#else
	main_window = glutCreateWindow ("Maxilla VRML viewer "PROGRAM_RELEASE" (C)2008-2013 by Zack Smith");
#endif

	//----------------------------------------
	// Set up pop up menu.
	//
	glutCreateMenu (handle_menu);
	glutAddMenuEntry ("Open...", 1);
	glutAddMenuEntry ("Close", 2);
	glutAddMenuEntry ("Help...", 3);
	glutAddMenuEntry ("Exit", 99);
	glutAttachMenu (GLUT_RIGHT_BUTTON);

	//----------------------------------------
	// Setup main window widgets & callbacks.
	//
	setup_main_window_widgets ();

	//----------------------------------------
	// Create second window.
	//
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize (1024, 760); 
	glutInitWindowPosition (5, 0);
	secondary_window = glutCreateWindow ("Maxilla "PROGRAM_RELEASE" (C)2008-201 by Ortho Cast Inc, Zack Smith");

	//----------------------------------------
	// Setup second window widgets & callbacks.
	//
	setup_secondary_window_widgets ();
	glutHideWindow ();

	//----------------------------------------
	if (file)
		load_file (file);

	//----------------------------------------
	glutMainLoop ();
	return 0; 
}

int 
Model::which_switch_has_triangle (Triangle *t)
{
	return -1;
}

void
IndexedFaceSet::ensure_tiny_triangles (double maximum)
{
	bool done = false;

#ifdef __APPLE__
	return;
#endif

	double maximum_squared_4 = 4. * maximum * maximum; // to avoid sqrt().
	
	//--------------------------------------------------
	// Loop until the maximum is completely enforced.
	//
	while (!done) {
		int i = 0;
		int n = n_triangles;
		int n2 = n;
		int p = n_points;
		//double avg_area = 0.f;
		int new_n_points = n_points;
		int new_n_triangles = n_triangles;

		//--------------------------------------------------
		// Go through the array of triangles, converting
		// big ones into four small ones each.
		//
		int n_large = 0;
		while (i < n) {
			Triangle *t = triangles [i];
			Point *p1 = t->p1;
			Point *p2 = t->p2;
			Point *p3 = t->p3;

			double v1x = p2->x - p1->x;
			double v1y = p2->y - p1->y;
			double v1z = p2->z - p1->z;
			double v2x = p3->x - p1->x;
			double v2y = p3->y - p1->y;
			double v2z = p3->z - p1->z;

			double v3x = v1y * v2z - v1z * v2y;
			double v3y = v1z * v2x - v1x * v2z;
			double v3z = v1x * v2y - v1y * v2x;

			//double area=0.5f * sqrt(v3x*v3x + v3y*v3y + v3z*v3z);
			double area = v3x*v3x + v3y*v3y + v3z*v3z;

			//avg_area += area;

			// if (0.5 * sqrt(...) >= maximim   .'.
			// if (sqrt(...) >= 2*maximum       .'.
			// if (... >= 4*maximum*maximum)
			//
			if (area >= maximum_squared_4) {
				n_large++;

				// OK! Need to create:
				// 3 new Points
				// 3 new Triangles (reuse the offending one).
				//
				Point *p12 = Point_new (
					(p1->x + p2->x) / 2.f,
					(p1->y + p2->y) / 2.f,
					(p1->z + p2->z) / 2.f);
				Point *p13 = Point_new (
					(p1->x + p3->x) / 2.f,
					(p1->y + p3->y) / 2.f,
					(p1->z + p3->z) / 2.f);
				Point *p23 = Point_new (
					(p3->x + p2->x) / 2.f,
					(p3->y + p2->y) / 2.f,
					(p3->z + p2->z) / 2.f);

				if ((new_n_points+3) >= points_size) {
					Point **points2 = (Point**) malloc (sizeof(Point*) * points_size * 2);
					for (int k=0; k < points_size; k++)
						points2 [k] = points [k];
					points_size *= 2;
					free (points);
					points = points2;
				}
				points [new_n_points++] = p12;
				points [new_n_points++] = p23;
				points [new_n_points++] = p13;

				Triangle *t1 = new Triangle (p1, p12, p13);
				Triangle *t2 = new Triangle (p2, p23, p12);
				Triangle *t3 = new Triangle (p3, p13, p23);
				Triangle *t4 = new Triangle (p12, p23, p13);
				t1->model = model;
				t2->model = model;
				t3->model = model;

				if ((new_n_triangles+3) >= triangles_size) {
					Triangle **triangles2 = (Triangle**) malloc (sizeof (Triangle**) * triangles_size * 2);
					for (int k = 0; k < triangles_size; k++)
						triangles2 [k] = triangles [k];
					triangles_size *= 2;
					free (triangles);
					triangles = triangles2;
				}

				triangles [i] = t1;
				triangles [new_n_triangles++] = t2;
				triangles [new_n_triangles++] = t3;
				triangles [new_n_triangles++] = t4;

#if 0
				Point *n = t->normal_vector;
				t1->normal_vector->x = n->x;
				t1->normal_vector->y = n->y;
				t1->normal_vector->z = n->z;
				t1->normal_vector->valid_vertex_normal = true;
				t2->normal_vector->x = n->x;
				t2->normal_vector->y = n->y;
				t2->normal_vector->z = n->z;
				t2->normal_vector->valid_vertex_normal = true;
				t3->normal_vector->x = n->x;
				t3->normal_vector->y = n->y;
				t3->normal_vector->z = n->z;
				t4->normal_vector->x = n->x;
				t4->normal_vector->y = n->y;
				t4->normal_vector->z = n->z;
#endif
				delete t;
			}

			i++;
		}

		if (!n_large)
			done = true;

 printf ("new_n_points = %d, n_points = %d\n", new_n_points, n_points);
 //printf ("new_n_triangles = %d, n_triangles = %d\n", new_n_triangles, n_triangles);
		n_points = new_n_points;
		n_triangles = new_n_triangles;

#if 0
		avg_area /= (double) n;
		printf ("Average triangle area = %g\n", avg_area);
		int pct = ((1000 * n_large + 5) / n) / 10;
		printf ("# triangles that were too large = %d (%d%%)\n", n_large, pct);
#endif
	}

}


/*===================================================================
 * Name:	serialize
 * Purpose:	Express the Node and its children as ASCII.
 */
void 
Node::serialize (gzFile f) 
{
	if (!f) 
		return;

	//----------------------------------------
	// Some types of nodes have their own
	// custom serialize routines, but some don't.
	// Therefore, we must inspect the type
	// and call the Type::serialize routine if
	// it exists and if not fall through to
	// normal Node serialization.
	//
	if (!strcmp (type, "Shape")) {
		Shape *obj = (Shape*) this;
		obj->serialize (f);
		return;
	}
	if (!strcmp (type, "IndexedFaceSet")) {
		IndexedFaceSet *obj = (IndexedFaceSet*) this;
		obj->serialize (f);
		return;
	}
	if (!strcmp (type, "IndexedLineSet")) {
		IndexedLineSet *obj = (IndexedLineSet*) this;
		obj->serialize (f);
		return;
	}
	if (!strcmp (type, "Replicated (USE)")) {
		Replicated *obj = (Replicated *) this;
		obj->serialize (f);
		return;
	}
	if (!strcmp (type, "Transform")) {
		Transform *obj = (Transform *) this;
		obj->serialize (f);
		return;
	}
	if (!strcmp (type, "Text")) {
		Text *obj = (Text*) this;
		obj->serialize (f);
		return;
	}
	
	//--------------------------------------------------
	// Only print name etc if we have a specific type,
	// i.e. not just the generic Node.
	//
	if (strcmp (type, "Node")) {
		indent (f);
		if (name) 
			gzprintf (f, " DEF %s %s {\n", name, type);
		else
			gzprintf (f, " %s {\n", type);
	} else {
		gzprintf (f, "#VRML V2.0 utf8\n\n\n");
	}

	++serialization_indentation_level;

	if (children) {
		int has_multiple = children->next? 1 : 0;

		if (strcmp (type, "Node")) {
			char *children_string = "children";
			if (!strcmp (type, "Switch"))
				children_string = "choice";

			indent (f);
			if (has_multiple)
				gzprintf (f, " %s [\n",
					children_string);
			else
				gzprintf (f, " %s\n",
					children_string);
		}

		++serialization_indentation_level;

		Node *n = children;
		while (n && !n->dont_save_this_node) {
			n->serialize (f);
			n = n->next;

			if (n && !n->dont_save_this_node) {
				indent (f);
				gzprintf (f, ",\n");
			}
		}

		--serialization_indentation_level;

		if (strcmp (type, "Node")) {
			if (has_multiple) {
				indent (f);
				gzprintf (f, " ]\n" );
			}
		}
	}

	if (!strcmp (type, "Switch")) {
		indent (f);
		gzprintf (f, "whichChoice %d\n", 
			((Switch*)this)->which );
	}

	if (strcmp (type, "Node")) {
		indent (f);
		gzprintf (f, " }\n" );
	}
	--serialization_indentation_level;
}

static bool write_positions_now = false;

//-----------------------------------------------------------------------------
// Name:	write_manual_spacing 
// Purpose:	Writes all manual spacing values to given file.
//-----------------------------------------------------------------------------
static void
write_manual_spacing (gzFile f)
{
	if (!manual_spacing_bottom)
		return;

	gzprintf (f, " string \"%g %g %g %g %g %g %g ",
		manual_spacing_bottom->translate_x,
		manual_spacing_bottom->translate_y,
		manual_spacing_bottom->translate_z,
		manual_spacing_bottom->rotate_x,
		manual_spacing_bottom->rotate_y,
		manual_spacing_bottom->rotate_z,
		manual_spacing_bottom->rotate_angle
		);

	int i;
	for (i=0; i<6; i++) {
		gzprintf (f, "%g ", manual_spacing_viewpoints [i]);
	}
	for (i=0; i<16; i++) {
		gzprintf (f, "%g ", manual_spacing_rotation [i]);
	}

	gzprintf (f, "\"\n");
}

void
Shape::serialize (gzFile f)
{
	indent (f);

	if (name)
		gzprintf (f, "DEF %s Shape {\n", name);
	else
		gzprintf (f, "Shape {\n");

	++serialization_indentation_level;

	if (appearance) {
		indent (f);
		gzprintf (f, "appearance\n");
		indent (f);
		gzprintf (f, "Appearance {\n");

		++serialization_indentation_level;

		indent (f);
		gzprintf (f, "material\n");
		
		indent (f);
		if (!name)
			gzprintf (f, "Material {\n");
		else
			gzprintf (f, "DEF %s Material {\n", name);

		indent (f);
		Material *m = appearance->material;

		if (m)
			gzprintf (f, " diffuseColor %g %g %g\n", 
				m->diffuseColor[0],
				m->diffuseColor[1],
				m->diffuseColor[2]);
		else
			gzprintf (f, " diffuseColor 1 0 0 \n");
		
		indent (f);
		gzprintf (f, "}\n");
		
		--serialization_indentation_level;

		indent (f);
		gzprintf (f, "}\n");
	}

	if (children) {
		// IndexedLineSet
		children->serialize (f);
	}

	--serialization_indentation_level;

	indent (f);
	gzprintf (f, "}\n");

	if (will_need_to_add_position_structures && write_positions_now &&
		did_manual_spacing)
	{
		gzprintf (f, "#------------------------------\n");

		char *position_str = "fubar";

		gzprintf (f, "DEF shape_manual_spacing Shape { geometry\n");
		gzprintf (f, " DEF manual_spacing Text { ");
		write_manual_spacing (f);
		gzprintf (f, "\n", position_str);
		gzprintf (f, " }\n");
		gzprintf (f, "}\n");

		gzprintf (f, "#------------------------------\n");
		write_positions_now = false;
	}
}

static int coord_num = 10;

void
IndexedFaceSet::serialize (gzFile f)
{
	indent (f);
	gzprintf (f, "geometry\n");
	indent (f);
	gzprintf (f, "IndexedFaceSet {\n");

	++serialization_indentation_level;

	//indent (f);
	//gzprintf (f, "# n_points %d, n_triangles %d\n", n_points, n_triangles);

	indent (f);
	gzprintf (f, "coord\n");
	indent (f);
	gzprintf (f, "DEF coord%d Coordinate { \n", coord_num++);

	++serialization_indentation_level;

	indent (f);
	gzprintf (f, "point [\n");

	++serialization_indentation_level;

	int point_counter = 0;
	int i;
	for (i = 0; i < n_points; i++) {
		Point *p = points[i];
		p->id = point_counter++;
	}

	for (i = 0; i < n_points; i++) {
		Point *p = points[i];

		indent (f);
		gzprintf (f, " %g %g %g ,\n", p->x, p->y, p->z);
	}

	indent (f);
	gzprintf (f, " ]\n");

	--serialization_indentation_level;

	indent (f);
	gzprintf (f, "}\n");
	indent (f);
	gzprintf (f, "coordIndex [ ");

	++serialization_indentation_level;

	int flag = 0;
	for (i = 0; i < n_triangles; i++) {
		Triangle *t = triangles [i];

		gzprintf (f, " %d , %d , %d , -1 , ", 
			t->p1->id,
			t->p2->id,
			t->p3->id);

		if (flag) {
			gzprintf (f, "\n");
			indent (f);
		}
		flag = !flag;
	}

	--serialization_indentation_level;

	indent (f);
	gzprintf (f, " ]\n");

	indent (f);
	gzprintf (f, " solid FALSE\n");

	--serialization_indentation_level;

	indent (f);
	gzprintf (f, "}\n");
}

void
IndexedLineSet::serialize (gzFile f)
{
	indent (f);
	gzprintf (f, "geometry IndexedLineSet {\n");

	++serialization_indentation_level;

	indent (f);
	gzprintf (f, "coord Coordinate { point [ ] }\n");

	indent (f);
	gzprintf (f, "color Color { color [ ] }\n");

	indent (f);
	gzprintf (f, "colorPerVertex TRUE\n");

	indent (f);
	gzprintf (f, "coordIndex [ ]\n");

	--serialization_indentation_level;

	indent (f);
	gzprintf (f, "}\n");
}


void
Replicated::serialize (gzFile f)
{
	indent (f);
	gzprintf (f, "USE %s \n", original->name ? original->name : "???");
}

void
Transform::serialize (gzFile f)
{
	//--------------------------------------------------
	// Only print name etc if we have a specific type,
	// i.e. not just the generic Node.
	//
	indent (f);
	if (name) 
		gzprintf (f, " DEF %s %s {\n", name, type);
	else
		gzprintf (f, " %s {\n", type);

	++serialization_indentation_level;

	if (children) {
		int has_multiple = children->next? 1 : 0;

		char *children_string = "children";
		if (!strcmp (type, "Switch"))
			children_string = "choice";

		indent (f);
		if (has_multiple)
			gzprintf (f, " %s [\n",
				children_string);
		else
			gzprintf (f, " %s\n",
				children_string);

		++serialization_indentation_level;

		Node *n = children;
		while (n && !n->dont_save_this_node) {
			n->serialize (f);
			n = n->next;

			if (n && !n->dont_save_this_node) {
				indent (f);
				gzprintf (f, ",\n");
			}
		}

		--serialization_indentation_level;

		if (has_multiple) {
			indent (f);
			gzprintf (f, " ]\n" );
		}
	}

	if (op_index) {
		int i;
		for (i = 0; i < op_index; i++) {
			indent (f);
			switch (operations[i]) 
			{
			case ROTATE:
				if (rotate_angle != 0.0f) 
					gzprintf (f, "rotation %g %g %g %g\n",
						rotate_x, rotate_y, rotate_z,
						rotate_angle);
				break;

			case ROTATE2:
				if (second_rotate_angle != 0.0f) 
					gzprintf (f, "rotation %g %g %g %g\n",
						second_rotate_x, second_rotate_y, second_rotate_z, second_rotate_angle);
				break;

			case SCALE:
				gzprintf (f, "scale %g %g %g\n",
					scale_x, scale_y, scale_z);
				break;
	
			case TRANSLATE:
				gzprintf (f, "translation %g %g %g\n",
					translate_x, translate_y, translate_z);
				break;

			case TRANSLATE2:
				gzprintf (f, "translation %g %g %g\n",
					second_translate_x, second_translate_y, second_translate_z);
				break;
			}
		}


	}
	
	--serialization_indentation_level;

	indent (f);
	gzprintf (f, " }\n" );
}

void
Text::serialize (gzFile f)
{
	indent (f);
	if (name) 
		gzprintf (f, "geometry DEF %s %s { ", name, type);
	else
		gzprintf (f, "geometry %s { ", type);

	indent (f);

	if (name && !strcmp ("manual_spacing", name)) 
		write_manual_spacing (f);
	else
		gzprintf (f, "string \"%s\"", text);

	indent (f);
	gzprintf (f, " }\n" );

	if (name && !strncmp ("Age_", name, 4))
		write_positions_now = true;
}

