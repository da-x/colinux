/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <colinux/common/common.h>

#include "apic.h"

co_rc_t co_manager_arch_init(co_manager_t *manager, co_archdep_manager_t *out_archdep);
{
	co_rc_t rc;

	*out_archdep = co_os_malloc(

	co_debug("manager: arch initialization\n");

	*out_archdep = co_os_malloc(
	
	rc = co_manager_arch_init_apic();

	return rc;
}
