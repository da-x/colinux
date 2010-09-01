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
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

#include <colinux/user/manager.h>
#include <colinux/user/daemon.h>
#include <colinux/user/debug/main.h>

COLINUX_DEFINE_MODULE("colinux-debug-daemon");

int main(int argc, char *argv[])
{
	co_rc_t rc;

	rc = co_debug_main(argc, argv);

	if (!CO_OK(rc)) {
		if (geteuid() != 0)
			printf ("Please run as root\n");
		return -1;
	}

	return 0;
}
