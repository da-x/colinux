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
#include <errno.h>

#include <colinux/os/user/exec.h>
#include <colinux/os/user/misc.h>

co_rc_t co_launch_process(int *pid, char *command_line, ...)
{
	char buf[0x100];
	char buf2[0x100];
	va_list ap;
	int ret;

	va_start(ap, command_line);
	co_vsnprintf(buf, sizeof(buf), command_line, ap);
	va_end(ap);

	snprintf(buf2, sizeof(buf2), "%s &", buf);

	co_debug("executing: %s", buf2);
	ret = system(buf2);

	if (ret == -1) {
		co_terminal_print("error in execution '%s' (%d)\n", buf2, errno);
		return CO_RC(ERROR);
	}


	if (pid)
		*pid = 0; /* dummy */

	return CO_RC(OK);
}

co_rc_t co_kill_process(int pid)
{
	return CO_RC(OK);
}
