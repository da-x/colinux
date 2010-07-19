/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <string.h>
#include <stdio.h>

#include <colinux/os/user/misc.h>
#include <colinux/user/manager.h>

#include "debug.h"

#ifdef COLINUX_DEBUG
static co_manager_handle_t handle = NULL;

void co_debug_start(void)
{
	if (handle != NULL)
		return;

	handle = co_os_manager_open_quite();
	if (handle) {
		co_rc_t rc;
		co_manager_ioctl_debug_levels_t levels = {{0}, };
		levels.modify = PFALSE;

		rc = co_manager_debug_levels(handle, &levels);
		if (CO_OK(rc)) {
			co_global_debug_levels = levels.levels;
		}
	}
}

void co_debug_buf(const char *buf, long size)
{
	if (handle != NULL)
		co_manager_debug(handle, buf, size);
}

void co_debug_end(void)
{
	co_manager_handle_t saved_handle = handle;

	handle = NULL;

	if (saved_handle != NULL)
		co_os_manager_close(saved_handle);
}

void co_debug_level_system(const char *module, co_debug_facility_t facility, int level,
		    const char *filename, int line, const char *func, const char *text)
{
	if (facility == CO_DEBUG_FACILITY_misc) {
		int len = strlen(text);
		char *format = (len > 0 && text[len-1] == '\n') ? "%s" : "%s\n";

		/* Warning: co_terminal_print here, would be recursive */
		fprintf(stderr, format, text);
	}
}
#else
void co_debug_start(void) { return; }
void co_debug_end(void) { return; }
#endif
