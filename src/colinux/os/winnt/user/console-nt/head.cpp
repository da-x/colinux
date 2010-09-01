/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

 /*
  * WinNT dependent main program for the text mode console.
  * Used for building colinux-console-nt.exe
  */
#include <windows.h>

extern "C" {
#include <colinux/common/debug.h>
#include <colinux/os/user/misc.h>
}

#include <colinux/user/console-nt/main.h>
#include "console.h"

COLINUX_DEFINE_MODULE("colinux-console-nt");

int main(int argc, char** argv)
{
	int status;

	global_window = new console_window_NT_t;

	if (global_window == NULL) {
		co_terminal_print("Unable to create console window");
		return -1;
	}

	try {
		status = co_user_console_main(argc, argv);
	} catch(...) {
		co_debug("The console program encountered an exception.");
		status = -1;
	}

	delete global_window;

	return status;
}
