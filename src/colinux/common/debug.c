/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "common.h"
#include "debug.h"

#include <stdarg.h>

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

void co_debug_(const char *module, int level, const char *filename, 
	       int line, const char *fmt, ...)
{
	char buf[0x100];
	int len_buf;
	va_list ap;

	if (level > 10)
		return;

	va_start(ap, fmt);
	co_snprintf(buf, sizeof(buf), "%s: ", module);
	len_buf = strlen(buf);
	co_vsnprintf(&buf[len_buf], sizeof(buf) - len_buf, fmt, ap);
	co_debug_line(buf);
	va_end(ap);
}


CO_TRACE_CONTINUE;

