/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_WINNT_KERNEL_MANAGER_H__
#define __CO_WINNT_KERNEL_MANAGER_H__

#include <colinux/os/kernel/manager.h>
#include <colinux/os/kernel/alloc.h>

#include "ddk.h"

typedef enum {
	CO_OS_MDL_PTR_TYPE_NONE=0,
	CO_OS_MDL_PTR_TYPE_MDL,
	CO_OS_MDL_PTR_TYPE_PAGE,
} co_os_mdl_ptr_type_t;

typedef struct co_os_mdl_ptr {
	co_os_mdl_ptr_type_t type;
	union {
		PMDL mdl;
		struct co_os_mdl_ptr *more_mdls;
		void *page;
	};
} co_os_mdl_ptr_t;

#define MDL_MAPPING_LEVELS         (3)
#define MDL_MAPPING_ENTRIES_SCALE  (10)
#define MDL_MAPPING_ENTRIES        (1 << MDL_MAPPING_ENTRIES_SCALE)

struct co_osdep_manager {
	co_os_mdl_ptr_t map_root;
	unsigned long mdls_allocated;
	unsigned long auxiliary_allocated;
	unsigned long auxiliary_peak_allocation;
	unsigned long mdls_peak_allocation;
	unsigned long blocks_allocated;
	unsigned long pages_mapped;
};

co_rc_t co_os_get_pfn_ptr(struct co_manager *manager, co_pfn_t pfn, co_os_mdl_ptr_t **mdl_out);

#endif
