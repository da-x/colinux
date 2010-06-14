/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_MANAGER_PAGES_H__
#define __COLINUX_KERNEL_MANAGER_PAGES_H__

#include "manager.h"

extern co_rc_t co_manager_get_page(struct co_manager *manager, co_pfn_t *pfn);
extern co_rc_t co_monitor_get_pfn(co_monitor_t *cmon, vm_ptr_t address, co_pfn_t *pfn);

extern co_rc_t co_monitor_copy_and_create_pfns(
	struct co_monitor *monitor,
	vm_ptr_t address,
	unsigned long size,
	char *source
	);

extern co_rc_t co_monitor_scan_and_create_pfns(
	struct co_monitor *monitor,
	vm_ptr_t address,
	unsigned long size
	);

extern co_rc_t co_monitor_create_ptes(
	struct co_monitor *monitor,
	vm_ptr_t address,
	unsigned long size,
	co_pfn_t *source
	);

extern co_rc_t co_monitor_copy_region(
	struct co_monitor *monitor,
	vm_ptr_t address,
	unsigned long size,
	void *data_to_copy
	);

extern co_rc_t co_monitor_alloc_and_map_page(
	struct co_monitor *monitor,
	vm_ptr_t address
	);

extern co_rc_t co_monitor_free_and_unmap_page(
	struct co_monitor *monitor,
	vm_ptr_t address
	);

#endif
