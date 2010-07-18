/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

/* WinNT host: allocate pages in WinNT kernel space */

#include "ddk.h"

#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/misc.h>

#include "manager.h"

static inline PMDL co_winnt_MmAllocatePagesForMdl(co_osdep_manager_t osdep)
{
	PHYSICAL_ADDRESS LowAddress  = { .QuadPart = 0x1000000 }; /* all under 16MB, reserved for DMA */
	PHYSICAL_ADDRESS HighAddress = { .QuadPart = osdep->hostmem_max_physical_address };
	PHYSICAL_ADDRESS SkipBytes   = { .QuadPart = 0 };

	return MmAllocatePagesForMdl(LowAddress,
				     HighAddress,
				     SkipBytes,
				     CO_ARCH_PAGE_SIZE * PFN_ALLOCATION_COUNT);
}

static co_rc_t co_winnt_new_mdl_bucket(co_osdep_manager_t osdep)
{
	co_os_mdl_ptr_t* mdl_ptr;
	co_os_pfn_ptr_t* pfn_ptrs;
	int 		 i;
	co_rc_t 	 rc;

	mdl_ptr = co_os_malloc(sizeof(*mdl_ptr));
	if (!mdl_ptr)
		return CO_RC(OUT_OF_MEMORY);

	pfn_ptrs = co_os_malloc(sizeof(co_os_pfn_ptr_t) * PFN_ALLOCATION_COUNT);
	if (!pfn_ptrs) {
		rc = CO_RC(OUT_OF_MEMORY);
	} else {
		mdl_ptr->use_count = 0;
		mdl_ptr->pfn_ptrs  = pfn_ptrs;
		mdl_ptr->mdl	   = co_winnt_MmAllocatePagesForMdl(osdep);

		if (mdl_ptr->mdl == NULL) {
			rc = CO_RC(OUT_OF_MEMORY);
		} else {
			int pages_allocated = mdl_ptr->mdl->ByteCount >> CO_ARCH_PAGE_SHIFT;

			if (pages_allocated != PFN_ALLOCATION_COUNT) {
				rc = CO_RC(OUT_OF_MEMORY);
			} else {
				osdep->mdls_allocated++;
				co_list_add_head(&mdl_ptr->node, &osdep->mdl_list);

				for (i = 0; i < PFN_ALLOCATION_COUNT; i++) {
					co_pfn_t pfn = ((co_pfn_t *)(mdl_ptr->mdl + 1))[i];

					pfn_ptrs[i].type = CO_OS_PFN_PTR_TYPE_MDL;
					pfn_ptrs[i].pfn  = pfn;
					pfn_ptrs[i].mdl  = mdl_ptr;
					co_list_add_head(&pfn_ptrs[i].unused, &osdep->pages_unused);
					co_list_add_head(&pfn_ptrs[i].node, &osdep->pages_hash[PFN_HASH(pfn)]);
				}

				return CO_RC(OK);

			}
			MmFreePagesFromMdl(mdl_ptr->mdl);
			IoFreeMdl(mdl_ptr->mdl);
		}
		co_os_free(pfn_ptrs);
	}
	co_os_free(mdl_ptr);
	return rc;
}

static co_rc_t co_winnt_new_mapped_allocated_page(co_osdep_manager_t osdep, co_pfn_t* pfn)
{
	co_os_pfn_ptr_t* pfn_ptr;
	void*            page;

	pfn_ptr = co_os_malloc(sizeof(*pfn_ptr));
	if (!pfn_ptr)
		return CO_RC(OUT_OF_MEMORY);

	page = MmAllocateNonCachedMemory(CO_ARCH_PAGE_SIZE);
	if (!page) {
		co_os_free(pfn_ptr);
		return CO_RC(OUT_OF_MEMORY);
	}

	pfn_ptr->type = CO_OS_PFN_PTR_TYPE_PAGE;
	pfn_ptr->pfn = co_os_virt_to_phys(page) >> CO_ARCH_PAGE_SHIFT;
	pfn_ptr->page = page;
	co_list_add_head(&pfn_ptr->node, &osdep->pages_hash[PFN_HASH(pfn_ptr->pfn)]);
	co_list_add_head(&pfn_ptr->mapped_allocated_node, &osdep->mapped_allocated_list);
	*pfn = pfn_ptr->pfn;

	return CO_RC(OK);
}

static void co_winnt_free_mapped_allocated_page(co_osdep_manager_t osdep, co_os_pfn_ptr_t* pfn_ptr)
{
	co_list_del(&pfn_ptr->mapped_allocated_node);
	co_list_del(&pfn_ptr->node);
	MmFreeNonCachedMemory(pfn_ptr->page, CO_ARCH_PAGE_SIZE);
	co_os_free(pfn_ptr);
}

static void co_winnt_free_mdl_bucket_no_lists(co_osdep_manager_t osdep, co_os_mdl_ptr_t* mdl_ptr)
{
	MmFreePagesFromMdl(mdl_ptr->mdl);
	IoFreeMdl(mdl_ptr->mdl);
	co_os_free(mdl_ptr->pfn_ptrs);
	co_list_del(&mdl_ptr->node);
	co_os_free(mdl_ptr);
	osdep->mdls_allocated--;
}

static void co_winnt_free_mdl_bucket(co_osdep_manager_t osdep, co_os_mdl_ptr_t* mdl_ptr)
{
	co_os_pfn_ptr_t* pfn_ptrs = mdl_ptr->pfn_ptrs;
	int 		 i;

	for (i = 0; i < PFN_ALLOCATION_COUNT; i++) {
		co_list_del(&pfn_ptrs[i].unused);
		co_list_del(&pfn_ptrs[i].node);
	}

	co_winnt_free_mdl_bucket_no_lists(osdep, mdl_ptr);
}

static void co_os_get_unused_mdl_page(co_osdep_manager_t osdep, co_pfn_t* pfn)
{
	co_os_pfn_ptr_t* pfn_ptr;

	pfn_ptr = co_list_entry(osdep->pages_unused.next, co_os_pfn_ptr_t, unused);
	pfn_ptr->mdl->use_count++;
	co_list_del(&pfn_ptr->unused);
	*pfn = pfn_ptr->pfn;
	osdep->pages_allocated++;
}

static void co_winnt_put_unused_mdl_page(co_osdep_manager_t osdep, co_os_pfn_ptr_t* pfn_ptr)
{
	co_os_mdl_ptr_t* mdl_ptr = pfn_ptr->mdl;

	mdl_ptr->use_count--;
	co_list_add_head(&pfn_ptr->unused, &osdep->pages_unused);
	if (mdl_ptr->use_count == 0)
		co_winnt_free_mdl_bucket(osdep, mdl_ptr);
	osdep->pages_allocated--;
}

void co_winnt_free_all_pages(co_osdep_manager_t osdep)
{
	co_os_mdl_ptr_t* mdl_ptr;
	co_os_pfn_ptr_t* pfn_ptr;

	while (!co_list_empty(&osdep->mdl_list)) {
		mdl_ptr = co_list_entry(osdep->mdl_list.next, co_os_mdl_ptr_t, node);
		co_winnt_free_mdl_bucket_no_lists(osdep, mdl_ptr);
	}

	while (!co_list_empty(&osdep->mapped_allocated_list)) {
		pfn_ptr = co_list_entry(osdep->mapped_allocated_list.next,
					co_os_pfn_ptr_t,
					mapped_allocated_node);
		co_winnt_free_mapped_allocated_page(osdep, pfn_ptr);
	}

	osdep->pages_allocated = 0;
}

co_rc_t co_os_get_page(struct co_manager* manager, co_pfn_t* pfn)
{
	co_osdep_manager_t osdep = manager->osdep;
	co_rc_t            rc    = CO_RC(OK);

	co_os_mutex_acquire(osdep->mutex);

	if (!co_list_empty(&osdep->pages_unused)) {
		co_os_get_unused_mdl_page(osdep, pfn);
	} else {

		rc = co_winnt_new_mdl_bucket(osdep);
		if (CO_OK(rc)) {
			co_os_get_unused_mdl_page(osdep, pfn);
		} else {
			rc = co_winnt_new_mapped_allocated_page(osdep, pfn);
		}
	}

	co_os_mutex_release(osdep->mutex);
	return rc;
}

void co_os_put_page(struct co_manager* manager, co_pfn_t pfn)
{
	co_osdep_manager_t osdep = manager->osdep;

	co_os_mutex_acquire(osdep->mutex);

	co_list_t*       list = &osdep->pages_hash[PFN_HASH(pfn)];
	co_os_pfn_ptr_t* pfn_ptr;

	co_list_each_entry(pfn_ptr, list, node) {
		if (pfn_ptr->pfn != pfn)
			continue;

		if (pfn_ptr->type == CO_OS_PFN_PTR_TYPE_MDL)
			co_winnt_put_unused_mdl_page(osdep, pfn_ptr);
		else
			co_winnt_free_mapped_allocated_page(osdep, pfn_ptr);

		break;
	}

	co_os_mutex_release(osdep->mutex);
}

void* co_os_map(struct co_manager* manager, co_pfn_t pfn)
{
	PVOID*           ret;
	PHYSICAL_ADDRESS PhysicalAddress;

	PhysicalAddress.QuadPart = pfn << CO_ARCH_PAGE_SHIFT;
	ret = MmMapIoSpace(PhysicalAddress, CO_ARCH_PAGE_SIZE, MmCached);

	manager->osdep->pages_mapped++;
	return ret;
}

void co_os_unmap(struct co_manager* manager, void* ptr, co_pfn_t pfn)
{
	MmUnmapIoSpace(ptr, CO_ARCH_PAGE_SIZE);
	manager->osdep->pages_mapped--;
}
