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

#include <colinux/arch/manager.h>
#include <colinux/arch/mmu.h>

#include "debug.h"

typedef struct co_osdep_manager *co_osdep_manager_t;

typedef enum {
	CO_MANAGER_STATE_NOT_INITIALIZED,
	CO_MANAGER_STATE_INITIALIZED_DEBUG,
	CO_MANAGER_STATE_INITIALIZED_ARCH,
	CO_MANAGER_STATE_INITIALIZED_OSDEP,
	CO_MANAGER_STATE_INITIALIZED,
} co_manager_state_t;

/*
 * The manager module manages the running coLinux's systems.
 */ 
typedef struct co_manager {
	co_manager_state_t state;

	struct co_monitor *monitors[CO_MAX_MONITORS];

	unsigned long hostmem_amount;
	unsigned long hostmem_used;
	unsigned long hostmem_usage_limit;
	unsigned long hostmem_pages;

	co_pfn_t *reversed_map_pfns;
	unsigned long reversed_page_count;
	unsigned long *reversed_map_pgds;
	unsigned long reversed_map_pgds_count;

	int monitors_count;
	co_osdep_manager_t osdep;
	co_archdep_manager_t archdep;

	co_manager_debug_t debug;
} co_manager_t;

struct co_monitor;

typedef struct co_manager_per_fd_state {
	struct co_monitor *monitor;
	co_debug_section_t *debug_section;
} co_manager_per_fd_state_t;

extern co_manager_t *co_global_manager;

extern co_rc_t co_manager_set_reversed_pfn(co_manager_t *manager, 
					   co_pfn_t real_pfn, co_pfn_t pseudo_pfn);
extern co_rc_t co_manager_load(co_manager_t *manager);
extern co_rc_t co_manager_ioctl(co_manager_t *manager, unsigned long ioctl, 
				void *io_buffer, unsigned long in_size,
				unsigned long out_size, unsigned long *return_size,
				void **private_data);
extern co_rc_t co_manager_cleanup(co_manager_t *manager, void **private_data);
extern void co_manager_unload(co_manager_t *manager);

#endif
