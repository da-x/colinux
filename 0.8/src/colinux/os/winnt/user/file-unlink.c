/*
 * This source code is a part of coLinux source package.
 *
 * Henry Nestler, 2007 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>
#include <colinux/os/user/file.h>

#include "misc.h"

co_rc_t co_os_file_unlink(co_pathname_t pathname)
{
	char last_error[0x100];

	if (!DeleteFile(pathname)) {
		co_winnt_get_last_error(last_error, sizeof(last_error));
		co_debug_error("Error unlink file %s: %s", pathname, last_error);
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}
