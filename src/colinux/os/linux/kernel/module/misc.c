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

co_rc_t co_os_physical_memory_pages(unsigned long *pages)
{
	*pages = num_physpages;

	/* 
	 * Round to 16 MB boundars, since Linux doesn't return the 
	 * exact amount but a bit lower.
	 */
	*pages = ~0xfff & ((*pages) + 0xfff);

	return CO_RC(OK);
}

__attribute__((regparm(0))) int regparm_check(int p1, int p2, int p3)
{
	if (p1 == 1 && p2 == 2 && p3 == 3)
		return 0;
	else
		return -EILSEQ;
}
