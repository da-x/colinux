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

#include <colinux/common/debug.h>

void co_vdebug(const char *format, va_list ap)
{
	char buf[0x100];
	vsnprintf(buf, sizeof(buf), format, ap);
	printf("%s", buf);
}

void co_vsnprintf(const char *format, va_list ap)
{
	char buf[0x100];
	vsnprintf(buf, sizeof(buf), format, ap);
	printf("%s", buf);
}

void co_snprintf(char *buf, int size, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, size, format, ap);
	va_end(ap);
}

void co_debug(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	co_vdebug(format, ap);
	va_end(ap);
}

void co_debug_line(char *line)
{
	printf("%s", line);
}
