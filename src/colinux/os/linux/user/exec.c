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

co_rc_t co_launch_process(char *command_line, ...)
{
	char buf[0x100];
	va_list ap;
	int ret;
	
	va_start(ap, command_line);
	vsnprintf(buf, sizeof(buf), command_line, ap);
	va_end(ap);

	co_debug("executing: %s\n", buf);
	ret = system(buf);

	if (ret == -1) {
		co_debug("error in execution (%d)\n", errno);
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}
