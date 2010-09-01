/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

 /*
  * OS independent main program for the text mode console.
  * Used for building colinux-console-nt.exe
  */

#include "console.h"
#include "main.h"

extern "C" {
#include <colinux/user/debug.h>
}

console_window_t* global_window = NULL;

void co_user_console_handle_scancode(co_scan_code_t sc)
{
	if (global_window == NULL)
		return;

	global_window->handle_scancode(sc);
}

int co_user_console_main(int argc, char** argv)
{
	co_rc_t rc;

	co_debug_start();

	rc = global_window->parse_args(argc, argv);
	if (!CO_OK(rc)) {
		co_debug("The console program was unable to parse the parameters.");
		goto co_user_console_main_error;
	}

	rc = global_window->start();
	if (!CO_OK(rc)) {
		co_debug("The console program could not start.");
		goto co_user_console_main_error;
	}

	do {
		rc = global_window->loop();
	} while (CO_OK(rc) && global_window->is_attached());

	if (global_window->is_attached())
		global_window->detach();

	if (CO_OK(rc)) {
		co_debug_end();
		return 0;
	}

co_user_console_main_error:
	co_debug("The console program encountered an error: %08x", (int)rc);

	co_debug_end();

	global_window = 0;

	return -1;
}
