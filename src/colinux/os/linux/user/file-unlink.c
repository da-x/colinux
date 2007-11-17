/*
 * This source code is a part of coLinux source package.
 *
 * Henry Nestler, 2007 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <unistd.h>

#include <colinux/os/user/file.h>

co_rc_t co_os_file_unlink(co_pathname_t pathname)
{
	if (unlink((char *)pathname) != 0)
		return CO_RC(ERROR);

	return CO_RC(OK);
}
