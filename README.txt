
This is Maxilla, which is a dentistry-specific VRML viewer.
It requires OpenGL, GLUT, GLUI, and zlib.

If you will build it under Linux, you must obtain and install FreeGLUT
before building Maxilla.

If you are using Mac OS/X, you need to obtain GLUI and build it,
then use Makefile-MacOSX.

Note, the Linux and Mac OS/X ports do not have file-open dialog
windows. You have to specify the VRML file on the command line.

Although this program is dentistry-specific, it could fairly 
easily be adapted to other purposes.
Be sure to remove "#define ORTHOCAST" from Maxilla.h.

Zack Smith
fbui@comcast.net

