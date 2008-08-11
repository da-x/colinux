/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */


#include <stdio.h>
#include <stdlib.h>

#include <colinux/os/user/manager.h>
#include <colinux/os/user/file.h>
#include <colinux/os/user/misc.h>
#include <colinux/user/cmdline.h>

#include "monitor.h"


co_rc_t read_pid_from_file(co_pathname_t pathname, co_id_t *id)
{
	char *buf;
	unsigned long size;
	co_rc_t rc;

	co_remove_quotation_marks(pathname);

	rc = co_os_file_load(pathname, &buf, &size, 32);
	if (!CO_OK(rc))
		return rc;

	if (size > 0) {
		*id = atoi(buf);
		co_debug("pid %d from file", (int)*id);
	} else {
		co_debug("file '%s' to small or unreadable", pathname);
		rc = CO_RC(INVALID_PARAMETER);
	}

	co_os_file_free(buf);
	return rc;
}
