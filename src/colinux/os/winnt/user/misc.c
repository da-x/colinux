/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/misc.h>

unsigned long co_os_get_physical_ram_size()
{
	MEMORYSTATUS memstat;
	
	GlobalMemoryStatus(&memstat);

	/* Round up in MBs: */

	return (memstat.dwTotalPhys + 0xFFFFF) & 0xFFF00000;
}

void co_terminal_print(const char *format, ...)
{
	char buf[0x100];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	printf("%s", buf);
}
