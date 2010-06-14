/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/file.h>

#include "misc.h"

co_rc_t co_os_file_load(co_pathname_t pathname, char **out_buf, unsigned long *out_size, unsigned long max_size)
{
	HANDLE handle;
	BOOL ret;
	unsigned long size, size_read, high;
	co_rc_t rc = CO_RC_OK;
	char *buf;

	handle = CreateFile(pathname,
			    GENERIC_READ,
			    FILE_SHARE_READ,
			    NULL,
			    OPEN_EXISTING,
			    FILE_ATTRIBUTE_NORMAL,
			    NULL);

	if (handle == INVALID_HANDLE_VALUE) {
		co_terminal_print_last_error(pathname);
		co_debug_error("Error opening file (%s)", pathname);
		rc = CO_RC(ERROR);
		goto out;
	}

	size = GetFileSize(handle, &high);

	if (max_size && (size > max_size || high))
		size = max_size;

	buf = (char *)malloc(size);
	if (buf == NULL)  {
		rc = CO_RC(OUT_OF_MEMORY);
		goto out2;
	}

	ret = ReadFile(handle, buf, size, &size_read, NULL);

	if (size != size_read) {
		co_terminal_print_last_error(pathname);
		co_debug_error("Error reading file %s, %lu != %lu",
				pathname, size, size_read);
		free(buf);
		rc = CO_RC(ERROR);
		goto out2;
	}

	*out_buf = buf;
	*out_size = size;

out2:
	CloseHandle(handle);
out:
	return rc;
}

void co_os_file_free(char *buf)
{
	free(buf);
}
