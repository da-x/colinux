/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "../ddk.h"
#include "../manager.h"

#include <colinux/common/debug.h>

void co_debug_system(const char *fmt, ...)
{
	char buf[0x100];
	va_list ap;

	va_start(ap, fmt);
	co_vsnprintf(buf, sizeof(buf), fmt, ap);
        DbgPrint("%s", buf);
	va_end(ap);
}
