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
#include <asm/io.h>
#include <colinux/os/kernel/misc.h>

unsigned long co_os_virt_to_phys(void *addr)
{
	return virt_to_phys(addr);
}

unsigned long co_os_current_processor(void)
{
	return smp_processor_id();
}

void co_snprintf(char *buf, int size, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, size, format, ap);
	va_end(ap);
}
