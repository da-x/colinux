/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "debug.h"

CO_TRACE_STOP;

void co_trace_ent_name(void *ptr, const char *name)
{
	static int reenter = 0;
	
	if (reenter)
		return;

	reenter++;
	
	co_debug("TRACE: [%08x] %s\n", ptr, name);

	reenter--;
}

CO_TRACE_CONTINUE;
