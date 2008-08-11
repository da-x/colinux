/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <libgen.h>
#include <colinux/os/user/cobdpath.h>

co_rc_t co_canonize_cobd_path(co_pathname_t *pathname)
{
	return CO_RC(OK);
}

co_rc_t co_dirname (char *path)
{
	dirname(path);

	return CO_RC(OK);
}
