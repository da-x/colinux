/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <linux/kernel.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#include <memory.h>

#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/monitor.h>
#include <colinux/os/kernel/manager.h>
#include <colinux/os/kernel/mutex.h>

#include "manager.h"
#include "monitor.h"

co_rc_t co_manager_load(co_manager_t *manager)
{
	co_rc_t rc;

	co_debug("manager: loaded to host kernel\n");

	manager->state = CO_MANAGER_STATE_NOT_INITIALIZED;
	memset(manager, 0, sizeof(*manager));

	rc = co_os_manager_init(manager, &manager->osdep);

	return rc;
}

static co_rc_t alloc_reversed_pfns(co_manager_t *manager)
{
	unsigned long map_size, covered_physical;
	unsigned long reversed_page_count;
	co_rc_t rc = CO_RC(OK);
	int i, j;

	co_debug("manager: allocating reversed physical mapping\n");
	manager->reversed_page_count = manager->host_memory_pages / PTRS_PER_PTE;
	map_size = sizeof(co_pfn_t) * manager->reversed_page_count;

	co_debug("manager: allocating top level pages map (%d bytes)\n", map_size);
	manager->reversed_map_pfns = (co_pfn_t *)co_os_malloc(map_size);
	if (manager->reversed_map_pfns == NULL)
		return CO_RC(ERROR);

	memset(manager->reversed_map_pfns, 0, map_size);
	
	co_debug("manager: allocating %d top level pages\n", manager->reversed_page_count);
	for (i=0; i < manager->reversed_page_count; i++) {
		co_pfn_t pfn;

		rc = co_os_get_page(manager, &pfn);
		if (!CO_OK(rc))
			return rc;

		manager->reversed_map_pfns[i] = pfn;
	}

	manager->reversed_map_pgds_count = 
		(((unsigned long)(-CO_VPTR_PHYSICAL_TO_PSEUDO_PFN_MAP) / PTRS_PER_PTE) / PTRS_PER_PGD) / sizeof(linux_pgd_t);

	co_debug("manager: using %d table entries for reversed physical mapping\n", manager->reversed_map_pgds_count);

	manager->reversed_map_pgds = (unsigned long *)(co_os_malloc(manager->reversed_map_pgds_count*sizeof(linux_pgd_t)));
	if (!manager->reversed_map_pgds) {
		if (!CO_OK(rc)) /* TODO: handle error */
			return rc;
	}

	covered_physical = 0;
	reversed_page_count = 0;
	for (i=0; i < manager->reversed_map_pgds_count; i++) {
		co_pfn_t pfn;
		linux_pte_t *pte;
	
		if (covered_physical >= manager->host_memory_pages) {
			manager->reversed_map_pgds[i] = 0; 
			continue;
		}
		
		rc = co_os_get_page(manager, &pfn);
		if (!CO_OK(rc))
			return rc;

		pte = (typeof(pte))(co_os_map(manager, pfn));
		for (j=0; j < PTRS_PER_PTE; j++) {
			co_pfn_t rpfn;
			if (reversed_page_count >= manager->reversed_page_count)
				break;

			rpfn = manager->reversed_map_pfns[reversed_page_count];
			pte[j] = pte_modify(__pte(rpfn << PAGE_SHIFT), __pgprot(__PAGE_KERNEL));
			reversed_page_count++;
		}
		co_os_unmap(manager, pte);

		manager->reversed_map_pgds[i] = (pfn << PAGE_SHIFT) | _KERNPG_TABLE; 

		covered_physical += (PTRS_PER_PTE * PTRS_PER_PGD * sizeof(linux_pgd_t));
	}

	return CO_RC(OK);
}

static void free_reversed_pfns(co_manager_t *manager)
{
	int i;

	if (manager->reversed_map_pfns) {
		for (i=0; i < manager->reversed_page_count; i++)
			co_os_put_page(manager, manager->reversed_map_pfns[i]);	

		co_os_free(manager->reversed_map_pfns);
	}

	if (manager->reversed_map_pgds) {
		for (i=0; i < manager->reversed_map_pgds_count; i++) {
			if (manager->reversed_map_pgds[i] != 0) {
				co_os_put_page(manager, manager->reversed_map_pgds[i] >> PAGE_SHIFT);
			}
		}

		co_os_free(manager->reversed_map_pgds);
	}
}

co_rc_t co_manager_set_reversed_pfn(co_manager_t *manager, co_pfn_t real_pfn, co_pfn_t pseudo_pfn)
{
	int entry, top_level;
	co_pfn_t *reversed_pfns;

	top_level = real_pfn / PTRS_PER_PTE;
	if (top_level >= manager->reversed_page_count)
		return CO_RC(ERROR);
		
	entry = real_pfn % PTRS_PER_PTE;

	reversed_pfns = (typeof(reversed_pfns))(co_os_map(manager, manager->reversed_map_pfns[top_level]));
	reversed_pfns[entry] = pseudo_pfn;
	co_os_unmap(manager, reversed_pfns);

	return CO_RC(OK);
}

void co_manager_unload(co_manager_t *manager)
{
	co_debug("manager: unloaded from host kernel\n");

	free_reversed_pfns(manager);
	co_os_manager_free(manager->osdep);
}

co_rc_t co_manager_init(co_manager_t *manager, void *io_buffer)
{
	co_manager_ioctl_init_t *params;
	co_rc_t rc = CO_RC(OK);

	params = (typeof(params))(io_buffer);

	manager->host_memory_amount = params->physical_memory_size;
	manager->host_memory_pages = manager->host_memory_amount >> PAGE_SHIFT;

	co_debug("manager: initialized (%d MB physical RAM)\n", manager->host_memory_amount/(1024*1024));

	rc = alloc_reversed_pfns(manager);

	manager->state = CO_MANAGER_STATE_INITIALIZED;

	return rc;
}

co_rc_t co_manager_cleanup(co_manager_t *manager, void **private_data)
{
	co_monitor_t *cmon = NULL;

	cmon = (typeof(cmon))(*private_data);
	if (cmon != NULL) {
		co_debug("manager: process exited abnormally, destroying attached monitor\n");

		co_monitor_destroy(cmon);

		*private_data = NULL;
	}
	
	return CO_RC(OK);
}

co_rc_t co_manager_ioctl(co_manager_t *manager, co_monitor_ioctl_op_t ioctl, 
			 void *io_buffer, unsigned long in_size,
			 unsigned long out_size, unsigned long *return_size,
			 void **private_data)
{
	co_rc_t rc = CO_RC_OK;
	co_monitor_t *cmon = NULL;

	*return_size = 0;

	switch (ioctl) {
	case CO_MANAGER_IOCTL_STATUS: {
		co_manager_ioctl_status_t *params;

		params = (typeof(params))(io_buffer);
		params->state = manager->state;
		params->monitors_count = manager->monitors_count;

		*return_size = sizeof(*params);

		return rc;
	}
	default:
		break;
	}

	if (manager->state == CO_MANAGER_STATE_NOT_INITIALIZED) {
		if (ioctl == CO_MANAGER_IOCTL_INIT) {
			return co_manager_init(manager, io_buffer);
		} else {
			co_debug("manager: invalid ioctl when not initialized\n");
			return CO_RC_ERROR;
		}

		return rc;
	}

	switch (ioctl) {
	case CO_MANAGER_IOCTL_CREATE: {
		co_manager_ioctl_create_t *params = (typeof(params))(io_buffer);

		params->rc = co_monitor_create(manager, params, &cmon);

		if (CO_OK(params->rc))
			*private_data = (void *)cmon;

		*return_size = sizeof(*params);

		return rc;
	}
	case CO_MANAGER_IOCTL_MONITOR: {
		co_manager_ioctl_monitor_t *params = (typeof(params))(io_buffer);
		*return_size = sizeof(*params);

		if (in_size < sizeof(*params)) {
			co_debug("manager: monitor ioctl too small! (%d < %d)\n", in_size, sizeof(*params));
			params->rc = CO_RC(MONITOR_NOT_LOADED);
			break;
		}
		
		cmon = (typeof(cmon))(*private_data);
		if (!cmon) {
			co_debug("manager: no monitor is attached\n");
			params->rc = CO_RC(MONITOR_NOT_LOADED);
			break;
		}
		
		in_size -= sizeof(*params);
		params->rc = co_monitor_ioctl(cmon, params, in_size, out_size,  return_size, private_data);
		break;

	}
	default:
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}
