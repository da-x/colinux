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

#include <colinux/os/kernel/misc.h>

unsigned long co_os_virt_to_phys(void *addr)
{
	PHYSICAL_ADDRESS pa;
    
	pa = MmGetPhysicalAddress((PVOID)addr);

	return pa.QuadPart;
}
