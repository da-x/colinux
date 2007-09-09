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

co_rc_t co_os_file_write(co_pathname_t pathname, void *buf, unsigned long size)
{
	HANDLE handle;
	BOOL ret;
	unsigned long wrote;
	co_rc_t rc = CO_RC_OK;
	char last_error[0x100];

	handle = CreateFile(pathname,
			    GENERIC_WRITE,
			    FILE_SHARE_READ,
			    NULL,
			    CREATE_ALWAYS,
			    FILE_ATTRIBUTE_NORMAL,
			    NULL);

	if (handle == INVALID_HANDLE_VALUE) {
		co_winnt_get_last_error(last_error, sizeof(last_error));
		co_debug_error("Error creating file (%s): %s", pathname, last_error);
		return CO_RC(ERROR);
	}

	ret = WriteFile(handle, buf, size, &wrote, NULL);
	if (ret == 0 || size != wrote) {
		co_winnt_get_last_error(last_error, sizeof(last_error));
		co_debug_error("Error write file %s: %s", pathname, last_error);
		rc = CO_RC(ERROR);
	}

	CloseHandle(handle);
	return rc;
}
