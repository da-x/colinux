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
#include <colinux/os/kernel/misc.h>

#include "manager.h"

co_rc_t co_os_get_page(struct co_manager *manager, co_pfn_t *pfn)
{
	PHYSICAL_ADDRESS LowAddress;
	PHYSICAL_ADDRESS HighAddress;
	PHYSICAL_ADDRESS SkipBytes;
	co_os_mdl_ptr_t *mdl_keeper;
	co_rc_t rc;
	PMDL mdl;

	LowAddress.QuadPart = 0x100000 * 16; /* >16MB We don't want to steal DMA memory */
	HighAddress.QuadPart = 0x100000000LL; /* We don't support PGE yet */
	SkipBytes.QuadPart = 0;

	mdl = MmAllocatePagesForMdl(LowAddress,
				    HighAddress,
				    SkipBytes,
				    PAGE_SIZE);
	if (mdl == NULL) {
		/*
		 * Using an alternative allocation method (limited to 256MB)
		 */
		void *page;
		
		page = co_os_alloc_pages(1);
		if (page == NULL) {
			co_debug("co_os_get_page: out of pages\n");
			return CO_RC(OUT_OF_PAGES);
		}
		
		*pfn = co_os_virt_to_phys(page) >> PAGE_SHIFT;
		
		rc = co_os_get_pfn_ptr(manager, *pfn, &mdl_keeper);
		if (!CO_OK(rc)) {
			co_os_free_pages(page, 1);
			return rc;
		}
		
		mdl_keeper->page = page;
		mdl_keeper->type = CO_OS_MDL_PTR_TYPE_PAGE; 

		manager->osdep->auxiliary_allocated++;

		if (manager->osdep->auxiliary_peak_allocation < manager->osdep->auxiliary_allocated) {
			manager->osdep->auxiliary_peak_allocation = manager->osdep->auxiliary_allocated;
		}

		return CO_RC(OK);
	}

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
	mdl_keeper->type = CO_OS_MDL_PTR_TYPE_MDL; 

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

	switch (mdl_keeper->type) {
	case CO_OS_MDL_PTR_TYPE_MDL: {
		if (mdl_keeper->mdl != NULL) {
			MmFreePagesFromMdl(mdl_keeper->mdl);
			IoFreeMdl(mdl_keeper->mdl);			
			manager->osdep->mdls_allocated--;
			mdl_keeper->mdl = NULL;
		}
		break;
	}
	case CO_OS_MDL_PTR_TYPE_PAGE: {
		if (mdl_keeper->page != NULL) {
			manager->osdep->auxiliary_allocated--;
			co_os_free_pages(mdl_keeper->page, 1);
			mdl_keeper->page = NULL;
		}
		break;
	}
	default:
		co_debug("co_os_get_page: bad mdl type for pfn %d\n", pfn);
		break;
	}
}
