/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <colinux/common/libc.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/monitor.h>
#include <colinux/os/kernel/manager.h>
#include <colinux/os/kernel/misc.h>
#include <colinux/os/kernel/mutex.h>
#include <colinux/arch/mmu.h>

#include "manager.h"
#include "monitor.h"
#include "pages.h"

co_manager_t *co_global_manager = NULL;

static co_rc_t alloc_reversed_pfns(co_manager_t *manager)
{
	unsigned long map_size, covered_physical;
	unsigned long reversed_page_count;
	co_rc_t rc = CO_RC(OK);
	int i, j;

	co_debug("allocating reversed physical mapping\n");
	manager->reversed_page_count = manager->host_memory_pages / PTRS_PER_PTE;
	map_size = sizeof(co_pfn_t) * manager->reversed_page_count;

	co_debug("allocating top level pages map (%d bytes)\n", map_size);
	manager->reversed_map_pfns = (co_pfn_t *)co_os_malloc(map_size);
	if (manager->reversed_map_pfns == NULL)
		return CO_RC(ERROR);

	co_memset(manager->reversed_map_pfns, 0, map_size);
	
	co_debug("allocating %d top level pages\n", manager->reversed_page_count);
	for (i=0; i < manager->reversed_page_count; i++) {
		co_pfn_t pfn;

		rc = co_manager_get_page(manager, &pfn);
		if (!CO_OK(rc))
			return rc;

		manager->reversed_map_pfns[i] = pfn;
	}

	manager->reversed_map_pgds_count = 
		(((unsigned long)(CO_VPTR_BASE - CO_VPTR_PHYSICAL_TO_PSEUDO_PFN_MAP) 
		  / PTRS_PER_PTE)  / PTRS_PER_PGD) / sizeof(linux_pgd_t);

	co_debug("using %d table entries for reversed physical mapping\n", manager->reversed_map_pgds_count);
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
		
		rc = co_manager_get_page(manager, &pfn);
		if (!CO_OK(rc))
			return rc;

		pte = (typeof(pte))(co_os_map(manager, pfn));
		for (j=0; j < PTRS_PER_PTE; j++) {
			co_pfn_t rpfn;
			if (reversed_page_count >= manager->reversed_page_count) {
				pte[j] = 0;
				continue;
			}

			rpfn = manager->reversed_map_pfns[reversed_page_count];
			pte[j] = (rpfn << CO_ARCH_PAGE_SHIFT) | (_PAGE_PRESENT | _PAGE_RW | _PAGE_DIRTY | _PAGE_ACCESSED);
			reversed_page_count++;
		}
		co_os_unmap(manager, pte, pfn);

		manager->reversed_map_pgds[i] = (pfn << CO_ARCH_PAGE_SHIFT) | _KERNPG_TABLE; 

		covered_physical += (PTRS_PER_PTE * PTRS_PER_PGD);
	}

	return CO_RC(OK);
}

co_rc_t co_manager_load(co_manager_t *manager)
{
	co_rc_t rc;

	co_memset(manager, 0, sizeof(*manager));

	co_debug("loaded to host kernel\n");

	rc = co_os_physical_memory_pages(&manager->host_memory_pages);
	if (!CO_OK(rc))
		return rc;

	if (manager->host_memory_pages > 0x100000) {
		co_debug("error, machines with more than 4GB are not currently supported\n");
		return CO_RC(ERROR);
	}

	co_debug("machine has %d MB of RAM\n", (manager->host_memory_pages >> 8));

	manager->host_memory_amount = manager->host_memory_pages << CO_ARCH_PAGE_SHIFT;
	manager->state = CO_MANAGER_STATE_INITIALIZED;

	rc = co_debug_init(&manager->debug);
	if (!CO_OK(rc))
		return rc;

	rc = co_manager_arch_init(manager, &manager->archdep);
	if (!CO_OK(rc))
		goto out_err_debug;

	rc = co_os_manager_init(manager, &manager->osdep);
	if (!CO_OK(rc))
		goto out_err_arch;

	rc = alloc_reversed_pfns(manager);
	if (!CO_OK(rc))
		goto out_err_os;

	return rc;

/* error path */
out_err_os:
	co_os_manager_free(manager->osdep);
	
out_err_arch:
	co_manager_arch_free(manager->archdep);

out_err_debug:
	co_debug_free(&manager->debug);
	return rc;
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
				co_os_put_page(manager, manager->reversed_map_pgds[i] >> CO_ARCH_PAGE_SHIFT);
			}
		}

		co_os_free(manager->reversed_map_pgds);
	}
}

co_rc_t co_manager_set_reversed_pfn(co_manager_t *manager, co_pfn_t real_pfn, co_pfn_t pseudo_pfn)
{
	int entry, top_level;
	co_pfn_t *reversed_pfns;
	co_pfn_t mapped_pfn;

	top_level = real_pfn / PTRS_PER_PTE;
	if (top_level >= manager->reversed_page_count)
		return CO_RC(ERROR);
		
	entry = real_pfn % PTRS_PER_PTE;

	mapped_pfn = manager->reversed_map_pfns[top_level];
	reversed_pfns = (typeof(reversed_pfns))(co_os_map(manager, mapped_pfn));
	reversed_pfns[entry] = pseudo_pfn;
	co_os_unmap(manager, reversed_pfns, mapped_pfn);

	return CO_RC(OK);
}

void co_manager_unload(co_manager_t *manager)
{
	co_debug("unloaded from host kernel\n");

	free_reversed_pfns(manager);
	co_os_manager_free(manager->osdep);
	co_manager_arch_free(manager->archdep);
	co_debug_free(&manager->debug);
}

static co_rc_t create_private_data(void **private_data, co_manager_per_fd_state_t **fd_state_out)
{
	co_manager_per_fd_state_t **fd_state;

	fd_state = (typeof(fd_state))private_data;

	if (*fd_state) {
		*fd_state_out = *fd_state;
		return CO_RC(OK);
	}

	*fd_state = (co_manager_per_fd_state_t *)co_os_malloc(sizeof(co_manager_per_fd_state_t));
	if (!*fd_state)
		return CO_RC(OUT_OF_MEMORY);

	(*fd_state)->monitor = NULL;
	(*fd_state)->debug_section = NULL;

	*fd_state_out = *fd_state;

	return CO_RC(OK);
}

co_rc_t co_manager_cleanup(co_manager_t *manager, void **private_data)
{
	co_manager_per_fd_state_t *fd_state;

	fd_state = ((typeof(fd_state))(*private_data));
	if (!fd_state)
		return CO_RC(OK);

	if (fd_state->monitor != NULL) {
		co_monitor_t *mon = fd_state->monitor;
		co_debug("process exited abnormally, removing attached monitor\n");
		fd_state->monitor = NULL;
		co_monitor_destroy(mon);
	}

	if (fd_state->debug_section != NULL) {
		co_debug_fold(&manager->debug, fd_state->debug_section);
		fd_state->debug_section = NULL;
	}

	co_os_free(fd_state);

	*private_data = NULL;
	
	return CO_RC(OK);
}

co_rc_t co_manager_ioctl(co_manager_t *manager, unsigned long ioctl, 
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
		params->periphery_api_version = CO_LINUX_PERIPHERY_API_VERSION;
		params->linux_api_version = CO_LINUX_API_VERSION;

		*return_size = sizeof(*params);
		return rc;
	}
	case CO_MANAGER_IOCTL_DEBUG: {
		co_manager_per_fd_state_t *fd_state = NULL;

		rc = create_private_data(private_data, &fd_state);

		if (CO_OK(rc))
			co_debug_write(&manager->debug, &fd_state->debug_section, io_buffer, in_size);

		return CO_RC(OK);
	}
	case CO_MANAGER_IOCTL_DEBUG_READER: {
		co_manager_ioctl_debug_reader_t *params;

		params = (typeof(params))(io_buffer);
		params->rc = co_debug_read(&manager->debug, params->user_buffer, 
					   params->user_buffer_size, &params->filled);

		*return_size = sizeof(*params);

		return CO_RC(OK);
	}
	default:
		break;
	}

	if (manager->state == CO_MANAGER_STATE_NOT_INITIALIZED) {
		return CO_RC_ERROR;
	}

	switch (ioctl) {
	case CO_MANAGER_IOCTL_CREATE: {
		co_manager_ioctl_create_t *params = (typeof(params))(io_buffer);

		params->rc = co_monitor_create(manager, params, &cmon);
		if (CO_OK(params->rc)) {
			co_manager_per_fd_state_t *fd_state = NULL;

			params->rc = create_private_data(private_data, &fd_state);
			if (CO_OK(params->rc))
				fd_state->monitor = cmon;
		}

		*return_size = sizeof(*params);

		return rc;
	}
	case CO_MANAGER_IOCTL_MONITOR: {
		co_manager_ioctl_monitor_t *params = (typeof(params))(io_buffer);
		co_manager_per_fd_state_t *fd_state;

		*return_size = sizeof(*params);

		if (in_size < sizeof(*params)) {
			co_debug("monitor ioctl too small! (%d < %d)\n", in_size, sizeof(*params));
			params->rc = CO_RC(MONITOR_NOT_LOADED);
			break;
		}
		
		fd_state = ((typeof(fd_state))(*private_data));
		if (!fd_state || !fd_state->monitor) {
			params->rc = CO_RC(MONITOR_NOT_LOADED);
			break;
		}
		
		in_size -= sizeof(*params);

		cmon = fd_state->monitor;
		params->rc = co_monitor_ioctl(cmon, params, in_size, out_size, return_size, fd_state);
		break;

	}
	default:
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}
