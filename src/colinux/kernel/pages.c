/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <colinux/os/kernel/alloc.h>

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
