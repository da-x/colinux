/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_MANAGER_H__
#define __COLINUX_KERNEL_MANAGER_H__

#include "monitor.h"
#include <colinux/common/ioctl.h>

/*
 * The manager module manages the running coLinux's systems.
 */ 
typedef enum {
	CO_MANAGER_STATE_NOT_INITIALIZED,
	CO_MANAGER_STATE_INITIALIZED,
} co_manager_state_t;

typedef struct co_manager {
	co_manager_state_t state;

	struct co_monitor *monitors[CO_MAX_MONITORS];

	unsigned long host_memory_amount;
	unsigned long host_memory_pages;
} co_manager_t;

extern co_rc_t co_manager_create_monitor(co_manager_t *manager, struct co_monitor **cmon);
extern void co_manager_destroy_monitor(struct co_monitor *cmon);
extern co_rc_t co_manager_get_monitor(co_manager_t *manager, co_id_t id, struct co_monitor **cmon);
extern void co_manager_put_monitor(struct co_monitor *cmon);
extern co_rc_t co_manager_ioctl(co_manager_t *manager, co_monitor_ioctl_op_t ioctl, 
				void *io_buffer, unsigned long in_size,
				unsigned long out_size, unsigned long *return_size);

#endif
