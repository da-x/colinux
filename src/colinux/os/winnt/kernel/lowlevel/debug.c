/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "../ddk.h"

#include <colinux/common/debug.h>

void co_debug_line(char *line)
{
	DbgPrint("%s", line);
}

void co_debug(const char *format, ...)
{
	/* Yuck! */
	DbgPrint(format, 
		 (unsigned long)((&format)[1]),
		 (unsigned long)((&format)[2]),
		 (unsigned long)((&format)[3]),
		 (unsigned long)((&format)[4]),
		 (unsigned long)((&format)[5]),
		 (unsigned long)((&format)[6]),
		 (unsigned long)((&format)[7]),
		 (unsigned long)((&format)[8]),
		 (unsigned long)((&format)[9]),
		 (unsigned long)((&format)[10]),
		 (unsigned long)((&format)[11]),
		 (unsigned long)((&format)[12]),
		 (unsigned long)((&format)[13]));
}
