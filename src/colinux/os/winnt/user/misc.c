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

co_rc_t co_os_get_physical_ram_size(unsigned long *mem_size)
{
	MEMORYSTATUSEX m;
	BOOL ret;

	m.dwLength = sizeof(m);
	ret = GlobalMemoryStatusEx(&m);
	if (ret != TRUE) {
		co_debug("Error, GlobalMemoryStatusEx returned: %x\n", GetLastError());
		return CO_RC(ERROR);
	}

	if (m.ullTotalPhys > (DWORDLONG)0xFF000000) {
		co_debug("Error, machines with more than 4GB are not currentlt supported\n");
		return CO_RC(ERROR);
	}

	/* Round by 16 MB */
	*mem_size = 0xFF000000 & (((unsigned long)m.ullTotalPhys) + 0xFFFFFF);

	return CO_RC(OK);
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
