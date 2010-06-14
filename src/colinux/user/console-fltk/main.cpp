/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "console.h"

static console_window_t* global_window = NULL;

console_window_t* co_user_console_get_window(void)
{
    return global_window;
}

void co_user_console_handle_scancode(co_scan_code_t sc)
{
	if (!global_window)
		return;

	global_window->handle_scancode(sc);
}

int co_user_console_main(int argc, char** argv)
{
	co_rc_t		 rc;
	console_window_t window;
	int ret;

	global_window = &window;

	co_debug_start();

	rc = window.parse_args(argc, argv);
	if (!CO_OK(rc)) {
		co_debug("Error %08x parsing parameters", (int)rc);
		ret = -1;
		goto out;
	}

	rc = window.start();
	if (!CO_OK(rc)) {
		co_debug("Error %08x starting console", (int)rc);
		ret = -1;
		goto out;
	}

	ret = Fl::run();
out:
	co_debug_end();

	return ret;
}
