/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <colinux/arch/mmu.h>
#include <colinux/os/kernel/alloc.h>

#include "monitor.h"

co_rc_t co_manager_get_page(struct co_manager *manager, co_pfn_t *pfn)
{
	co_rc_t rc = CO_RC_OK;

	rc = co_os_get_page(manager, pfn);
	if (!CO_OK(rc))
		return rc;

	if (*pfn >= manager->host_memory_pages) {
		/* Surprise! We have a bug! */

		co_debug("co_manager_get_page: PFN too high! %d >= %d",
			 *pfn, manager->host_memory_pages);

		return CO_RC(ERROR);
	}

	return rc;
}

co_rc_t co_monitor_get_pfn(co_monitor_t *cmon, vm_ptr_t address, co_pfn_t *pfn)
{
	unsigned long current_pfn, pfn_group, pfn_index;

	current_pfn = (address >> PAGE_SHIFT);
	pfn_group = current_pfn / PTRS_PER_PTE;
	pfn_index = current_pfn % PTRS_PER_PTE;

	if (cmon->pp_pfns[pfn_group] == NULL)
		return CO_RC(ERROR);

	*pfn = cmon->pp_pfns[pfn_group][pfn_index];

	return CO_RC(OK);
}
