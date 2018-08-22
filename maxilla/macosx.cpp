/*=============================================================================
 * Maxilla, an OpenGL-based program for viewing dentistry-related VRML & STL.
 * Copyright (C) 2010 by Zack T Smith and Ortho Cast Inc.
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

#ifdef __APPLE__

#include <CoreFoundation/CFString.h>
#include <Carbon/Carbon.h>
#include <stdio.h>
#include <string.h>

#define ENTRY fprintf (stderr, "Entered %s\n", __FUNCTION__);

void
macosx_color_chooser ()
{
	// XX
}

void
macosx_message_box (char *t, char *s)
{
	CFStringRef title = CFStringCreateWithCString (NULL, t, kCFStringEncodingUTF8);
	CFStringRef msg = CFStringCreateWithCString (NULL, s, kCFStringEncodingUTF8);
	CFUserNotificationDisplayNotice (0, kCFUserNotificationPlainAlertLevel,
		NULL, NULL, NULL, title, msg, CFSTR ("OK"));
	CFRelease (title);
	CFRelease (msg);
}

int
macosx_file_open (char *path, int len)
{
	int retval = 0;
	OSStatus status;

	NavDialogCreationOptions options;
	status = NavGetDefaultDialogCreationOptions (&options);
	if (status == noErr) {
		options.windowTitle = CFSTR("Open");
		options.message = CFSTR("Select a file to open");
		options.modality = kWindowModalityAppModal;
		options.saveFileName = CFStringCreateWithCString (NULL, "Hello.wrl", kCFStringEncodingMacRoman);

		NavDialogRef dialog;
		status = NavCreateGetFileDialog (&options, NULL, NULL, NULL, NULL, NULL, &dialog);
		if (status == noErr && dialog != NULL) {
			status = NavDialogRun (dialog);
			if (status == noErr) {
				NavReplyRecord reply;
				status = NavDialogGetReply (dialog, &reply);
				if (status == noErr && reply.validRecord) {
					AEDesc aedesc;
					status = AECoerceDesc (&reply.selection, typeFSRef, &aedesc);
					if (status == noErr) {
						FSRef p;
						status = AEGetDescData (&aedesc, &p, sizeof(FSRef));
						if (status == noErr) {
							FSRefMakePath (&p, (UInt8*) path, len);
							retval = 1;

printf ("User specified file: %s\n",path);
fflush (stdout);
						}
						else
							puts ("Can't get path.");
					}
					NavDisposeReply (&reply);
				}
				else
					puts ("Unable to get data out of Open Dialog.");
			} else 
				puts ("Dialog run failed.");

			NavDialogDispose (dialog);
		}
	}

	return retval;
}

int
macosx_file_save (char *path, int len)
{
ENTRY
	int retval = 0;
	OSStatus status;

	OSType filetype, filecreator;
	NavDialogRef dialog;
	NavDialogCreationOptions dialog_options;

	NavGetDefaultDialogCreationOptions (&dialog_options);

	status = NavCreatePutFileDialog (&dialog_options, 
		NULL,
		NULL,
		NULL, NULL, &dialog);

	if (status == noErr && dialog != NULL) {
		status = NavDialogRun (dialog);
		if (status == noErr) {
			NavReplyRecord reply;
			status = NavDialogGetReply (dialog, &reply);
			if (status == noErr && reply.validRecord) {
				AEDesc aedesc;
				status = AECoerceDesc (&reply.selection, typeFSRef, &aedesc);
				if (status == noErr) {
					FSRef p;
					status = AEGetDescData (&aedesc, &p, sizeof(FSRef));
					if (status == noErr) {
						FSRefMakePath (&p, (UInt8*) path, len);
						retval = 1;

						strcat (path, "/");

						const UniChar *ptr = CFStringGetCharactersPtr (
							reply.saveFileName);

char filename [1000];
Boolean b = CFStringGetCString (reply.saveFileName, filename, 999, kCFStringEncodingMacRoman);  

						strcat (path, filename);
						//strcat (path, "output.pdf");
printf ("User specified file: %s\n",path);
fflush (stdout);
					}
					else
						puts ("Can't get path.");
				}
				NavDisposeReply (&reply);
			}
			else
				puts ("Unable to get data out of Open Dialog.");
		} else 
			puts ("Dialog run failed.");

		NavDialogDispose (dialog);
	}

	return retval;
}

#endif
