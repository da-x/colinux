/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <asm/page.h>

#include "ddk.h"

#include <colinux/os/kernel/alloc.h>

#include "manager.h"

co_rc_t co_os_get_page(struct co_manager *manager, co_pfn_t *pfn)
{
	PHYSICAL_ADDRESS LowAddress;
	PHYSICAL_ADDRESS HighAddress;
	PHYSICAL_ADDRESS SkipBytes;
	co_os_mdl_ptr_t *mdl_keeper;
	co_rc_t rc;
	PMDL mdl;
	unsigned long tries = 0;

	LowAddress.QuadPart = 0x100000 * 16; /* >16MB We don't want to steal DMA memory */
	HighAddress.QuadPart = 0x100000000LL; /* We don't support PGE yet */
	SkipBytes.QuadPart = 0;

retry:
	mdl = MmAllocatePagesForMdl(LowAddress,
				    HighAddress,
				    SkipBytes,
				    PAGE_SIZE);
	if (mdl == NULL) {
		tries++;
		if (tries < 1) {
			goto retry;
		}
		co_debug("co_os_get_page: MDL allocation error\n");
		return CO_RC(OUT_OF_PAGES);
	}
	if (tries != 0)
		co_debug("co_os_get_page: MDL okay after %d tries\n", tries);

	manager->osdep->mdls_allocated++;

	if (manager->osdep->mdls_peak_allocation < manager->osdep->mdls_allocated) {
		manager->osdep->mdls_peak_allocation = manager->osdep->mdls_allocated;
	}
	
	*pfn = ((unsigned long *)(mdl+1))[0];

	rc = co_os_get_pfn_ptr(manager, *pfn, &mdl_keeper);
	if (!CO_OK(rc)) {
		manager->osdep->mdls_allocated--;
		MmFreePagesFromMdl(mdl);
		IoFreeMdl(mdl);
		return rc;
	}

	if (mdl_keeper->mdl != NULL) {
		co_debug("co_os_get_page: %d is already allocated\n", *pfn);
		manager->osdep->mdls_allocated--;
		MmFreePagesFromMdl(mdl);
		IoFreeMdl(mdl);
		return CO_RC(ERROR);
	}

	mdl_keeper->mdl = mdl;
	return CO_RC(OK);
}

void *co_os_map(struct co_manager *manager, co_pfn_t pfn)
{
	PVOID *ret;
	PHYSICAL_ADDRESS PhysicalAddress;

	PhysicalAddress.QuadPart = pfn << PAGE_SHIFT;
	ret = MmMapIoSpace(PhysicalAddress, PAGE_SIZE, MmCached);

	manager->osdep->pages_mapped++;
	return ret;
}

void co_os_unmap(struct co_manager *manager, void *ptr)
{
	MmUnmapIoSpace(ptr, PAGE_SIZE);
	manager->osdep->pages_mapped--;
}

void co_os_put_page(struct co_manager *manager, co_pfn_t pfn)
{
	co_os_mdl_ptr_t *mdl_keeper;
	co_rc_t rc;

	rc = co_os_get_pfn_ptr(manager, pfn, &mdl_keeper);
	if (!CO_OK(rc))
		return;

	MmFreePagesFromMdl(mdl_keeper->mdl);
	IoFreeMdl(mdl_keeper->mdl);

	mdl_keeper->mdl = NULL;
	manager->osdep->mdls_allocated--;
}
