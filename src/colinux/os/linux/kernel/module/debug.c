/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include "linux_inc.h"

#include <colinux/common/debug.h>

void co_vdebug(const char *format, va_list ap)
{
	char buf[0x100];
	vsnprintf(buf, sizeof(buf), format, ap);
	printk("%s", buf);
}

void co_debug(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	co_vdebug(format, ap);
	va_end(ap);
}
