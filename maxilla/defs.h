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
 * Please do not use multiple inheritance or nested templates in this program.
 * They are the leading causes of C++ code illegibility.
 *
 * Keep this program clean! Don't pollute it.
 */

/*
 * Additional intermittent coding by KB and Josh (as noted in Changes).
 */

//----------------------------------------------------------------------------
// Changes:
// 0.2  Added data from .wrl file as C array.
// 0.3  Added C++ classes for Point, Triangle, Model.
// 0.4  Added code to compute normal vectors.
// 0.5  Added color-change logic & improved popups.
// 0.6  Class refinements. Added GLUI button.
// 0.7  Added window title logic. Added animation.
// 0.8  Added orthographic mode as default.
// 0.9  Improved orthographic, GUI now tells which perspective.
// 0.10 More animation.
// 0.11 Added VRML reader. Default model used if none specified.
// 0.12 Added some VRML parsing.
// 0.13	Added more VRML parsing. Title widget is affected.
// 0.14 Added more VRML parsing. Field of view widget affected.
// 0.15 First version that displays VRML data (IndexedFaceSet only).
// 0.16 Added basic support for VRML appearance, material, sphere.
// 0.17 Added multiple input file handling, open-file dialog.
// 0.18 Added GUI improvements; benchmarking of file load.
// 0.19 Added mouse-based rotation of model.
// 0.20 Removed ability to change colors: model provides those.
// 0.21 Added code to ascertain bounding box for all objects.
// 0.22 Added auto-translate of off-center model to (0,0,0).
// 0.23 Added support for Transform rotation.
// 0.24 Added ability to switch to user-specified colors.
// 0.25 Removed animation. Added quaternion-based rotation ball widget.
// 0.26 Combined subwindow with main window.
// 0.27 Improvements in rotation and perspective.
// 0.28 Added pop-up menus. 
// 0.29 Improvements to orthographic lighting. 
// 0.30 Added ability to read gzipped files.
// 0.31 Added input file read buffering for speed-up.
// 0.32 Refinement in color usage.
// 0.33 Further refinement of user-alterable lighting.
// 0.34	More efficient file reading by avoiding some mallocs.
// 0.35 Added ability to parse Group construct.
// 0.36 Added ability to parse Text. Patient data now displaying.
// 0.37 Added ability to parse Switch construct.
// 0.38 Added support for IndexedLineSet.
// 0.39 Modified software to utilize DST files' switchNode.
// 0.40 Improvements to OpenGL expression and bounds reporting.
// 0.41 Improved rotations bounds check.
// 0.42 Added Win32 Registry access functions.
// 0.43 Occlusal orientation now works with maxilla-only view.
// 0.44 Added ability to change preset foreground & background colors.
// 0.45 Added rudimentary selection logic.
// 0.46 Improved selection logic: It now places red ball where user clicked.
// 0.47 Added color handling with multi-line.
// 0.48 Added XY cross-section.
// 0.49 Added XZ cross-section; revised WhiteLight to support another position.
// 0.50 Simplified choice of views using GLUI listbox.
// 0.51 Code clean-up prior to first Linux release.
// 0.52 [By KB:] Added smoothing via adjacent-face normals.
// 0.53 Removed KB's adjacent-face logic.
// 0.54 Implemented smoothing to be nearly instantaneous.
// 0.55 Fixed excess smoothing near hard edges.
// 0.56 Moved location at which smoothing occurs to inside Model::express ().
// 0.57 [By KB:] Added print_pixels (), which writes floating point #s. 
// 0.58 Added crease detection and altered smoothing for points along creases.
// 0.59 Vertex normals are now weighted based on triangle area. 
// 0.60 Removed print_pixels () and replaced it w/bmplib-based screen_dump ().
// 0.61 Windows console window is now hidden when DEBUG is not #define'd.
// 0.62 Alteration of menus to add buttons for occlusal and back views.
// 0.63 Implemented occlusal view kludge: constructs new view after parsing.
// 0.64 Added a 0.5 inch space between maxilla & mandible in occusal view.
// 0.65 Changed occlusal view to checkbox, improved switching behavior.
// 0.66 Added preliminary auto setting of scale_factor (occlusal needs work).
// 0.67 Mouse motion now uses a quaternion instead of axis rotations.
// 0.68 Fixed memory leak, added some const correctness.
// 0.69 Added quaternions, which are now at the core of rotation logic.
// 0.70 Converted quaternion code to class.
// 0.71 Added buttons for increasing/decreasing occlusal space.
// 0.72 Revised handling of Appearance & Material to support "appearance USE".
// 0.73 Added support for Cone.
// 0.74 Removed double buffering.
// 0.75 Added my httplib code.
// 0.76 Added red_dot_stack logic, including ability to have any # of dots.
// 0.77 Revised copyright info.
// 0.78	Fixed color save.
// 0.79	[Josh:] Added arbitrary (x,y,0) translation.
// 0.80	Bug fix.
// 0.81	Removed Windows Registry code. Config params now stored to $HOMEDIR.
// 0.82	Added binary-STL file parser.
// 0.83	Added code to read two associated dentistry STLs.
// 0.84 Re-enabled cross sections. Fixed bug preventing adding dots to STLs.
// 0.85 In cross section of DST file, upper is now green & lower is red.
// 0.86	Cross section now puts tiny ball at the center of each triangle.
// 0.87	Cross section revised, GUI controls cleaned up.
// 0.88	Got Maxilla compiling under Linux again (Slackware64).
// 0.89	Fixes user-param bug under 64-bit Linux.
// 0.90 Improvements to filled-in cross section.
// 0.91	Better differentiation between occlusal and manual spacing modes.
// 0.92	Initial update of program for Mac OS/X.
// 0.93	Added equivalent of MessageBox to Mac OS/X code.
// 0.94	Rearranged widgets.
// 0.95	More rearrangements of widgets.
// 0.96	Implemented multi-view. Fixed measuring problems.
// 0.97	Added two additional subwindows and fixed aspect ratio problem.
// 0.98	Made it possible to view only upper or lower in subwindow.
// 0.99	Revised subwindow angles to be left-right-upper, right-back-lower.
// 0.100	Added support for first and last names.
// 0.101	Added support for control number in DST file.
// 0.102	Fixed bug to do with maxilla/mandible checkboxes not working.
// 0.103	Fixed minor bug.
// 0.104	Imposed rule that cutaway, occlusal, and manual spacing are
//		mutually exclusive.
// 0.105	Added simple PDF printing.
// 0.106	Added ability to set doctor/practice name & have it retained.
// 0.107	Practice name, patient name & case# are now drawn in PDF.
// 0.108	[KB:] Added ability to rotate lower jaw.
// 0.112	Addition & refinement of PDF printing.
// 0.113	Minor adjustments.
// 0.114	Fixed file open / pdf save multiple popups problem.
// 0.115	PDF now goes to temp file & is opened. Help launches browser.
// 0.116	Populated secondary window with buttons.
// 0.117	Added "Save PDF" alongside Print button. 
// 0.118	Calibration. Added dumping of pixels.txt for PDF debugging.
// 0.119	Help button now invokes default web browser rather than IE.
// 0.120	Revised single-view printing to make it precisely 1:1.
// 0.121	Got Bolton screen's widgets working.
// 0.122	Got measuring in Bolton working.
// 0.123	Fixed IndexedFaceSet color bug.
// 0.124	Got ensure_tiny_triangles() working. Order of points matters!
// 0.125	Got red-dot movement working during measuring.
// 0.126	Fixes measurment issue.
// 0.127	Improved secondary screen, removed "mm" where redundant.
// 0.128	Changed coordinates to doubles. Optimized ensure_tiny_triangles().
// 0.129	Added diagnostics.
// 0.130	More diagnostics.
// 0.131	Improvements to handle_resize. Removed handle_idle. More diags.
// 0.132	Improved detail in PDF printouts.
// 0.133	Delayed printing to get around the problem of glutHideWindow 
//	not acting to hide subwindow until callback returns.
// 0.134	Got it compiling under Linux again, however subwindows aren't
//	supported by Mesa3D so multiview mode doesn't work.
// 0.135	Mostly fixed problem with models in PDF being cut off.
// 0.136	Fixed issue when in multiview & switching to secondary screen.
// 0.137	Open-file routine is now in a thread for better GUI behavior.
// 0.138	Fix for load_file not reverting to single-view. Fix for blank PDFs.
// 0.139	Serialization of frame grabs! Needed for more primitive OpenGL's
//	and in particular Windows XP.
// 0.140	Got invocation of Acrobat working again.
// 0.141	Removed pop up window.
// 0.142	Added measuring lines.
// 0.143	Limited measuring lines' length. Set their x value to edge of
//	cutaway. Also compressed buttons and moved doctors office info left.
// 0.144	Moved multiview subwindows contents around, removed lower-center.
// 0.145,146	Various.
// 0.147	Made 2nd measuring line green. Difference can now be negative.
// 0.148	Measuring lines
// 0.149	Measuring lines
// 0.150	Measuring lines & rudimentary bolton printing.
// 0.151	Fixed lines snapping back to zero problem.
// 0.152	Workaround for GLUI's set_float_val not working w.r.t. xlation
//		widget.
// 0.153	Corrected Bolton ratio calculation.
// 0.154	First attempt at writing VRML to disk.
// 0.155	First point at which the written VRML can be read back in.
// 0.156	Transforms now work, so read-in written VRML looks normal.
// 0.157	Text now works, so the patient data is written & read correctly.
// 0.158	Compression of written dst file.
// 0.159	Added detection of DST by doing a search for the Switch node.
// 0.160	Implemented more complete write/read of manual spacing information.
// 0.161	DST opens in manual spacing mode if file has that info,
//		and save-as saves manual spacing values only if there is info
//		to save.
// 0.162	Implemented Mac save popup.
// 0.163	Got Mac measuring working.
// 0.164	Converted Point class to C due to poor C++ results on OS/X.
// 0.165	??
// 0.166	Fixed selection y value.
// 0.167	Fixed selection in Bolton. Fixed measurment checkbox not cleared
//			after returning from Bolton.
// 0.168	Disabled smoothing for the newer, highly-detailed DSTs.
// 0.169	Added save as jpg for windows only, relies on image magic convert util
// 0.170	Added second view layout for occlusal 
// 0.171	Added download models button
// 0.172	Increased allocation values in smooth function to avoid memory corruption
// 0.173	STL export
// 0.174	STL export support for model transformations
// 0.175	rear view made visible in multiview mode
// 0.176	Fixed multiview print size. Now allowing multiple successive prints.
//            Fixed Print / Save PDF crash when the PDF is open/locked.
// 0.177	Fixed trailing comma after patient name in PDF.
//----------------------------------------------------------------------------

#ifndef _DEFS_H
#define _DEFS_H

#define PROGRAM_RELEASE "0.177"

#define ORTHOCAST

#ifndef PATH_MAX
#define PATH_MAX (4096)
#endif

#define INPUTFILE_BUFFERSIZE (131072)
#define INITIAL_TRIANGLES_ARRAY_SIZE (16)

#define RGB_RED (0xff0000)
#define RGB_GREEN (0xff00)
#define RGB_BLUE (0xff)

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include "GL/glui.h"
#else
#include <GL/gl.h> 
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glui.h>
#endif

#ifdef __CYGWIN__
#include <w32api/windows.h>
#include <w32api/winuser.h>
#endif

extern unsigned long total_allocated;

extern void fatal (const char*);
extern void notice(char*);
extern void warning(char*);
extern void fatal_null_pointer(const char*,char*);
extern bool diag_write (char *str);

extern bool did_manual_spacing;// Determines whether spacing info saved to file.

#endif /* _DEFS_H */
