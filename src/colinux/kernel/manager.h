/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_MANAGER_H__
#define __COLINUX_KERNEL_MANAGER_H__

#include "monitor.h"

#include <colinux/common/ioctl.h>
#include <colinux/arch/manager.h>

typedef struct co_osdep_manager *co_osdep_manager_t;

/*
 * The manager module manages the running coLinux's systems.
 */ 
typedef struct co_manager {
	co_manager_state_t state;

	struct co_monitor *monitors[CO_MAX_MONITORS];

	unsigned long host_memory_amount;
	unsigned long host_memory_pages;

	co_pfn_t *reversed_map_pfns;
	unsigned long reversed_page_count;
	unsigned long *reversed_map_pgds;
	unsigned long reversed_map_pgds_count;

	int monitors_count;
	co_osdep_manager_t osdep;
	co_archdep_manager_t archdep;
} co_manager_t;

extern co_rc_t co_manager_set_reversed_pfn(co_manager_t *manager, 
					   co_pfn_t real_pfn, co_pfn_t pseudo_pfn);
extern co_rc_t co_manager_load(co_manager_t *manager);
extern co_rc_t co_manager_ioctl(co_manager_t *manager, co_monitor_ioctl_op_t ioctl, 
				void *io_buffer, unsigned long in_size,
				unsigned long out_size, unsigned long *return_size,
				void **private_data);
extern co_rc_t co_manager_cleanup(co_manager_t *manager, void **private_data);
extern void co_manager_unload(co_manager_t *manager);

#endif
