/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __COLINUX_USER_MONITOR_H__
#define __COLINUX_USER_MONITOR_H__

#include <colinux/common/common.h>
#include <colinux/common/console.h>
#include <colinux/common/ioctl.h>
#include <colinux/os/user/manager.h>

typedef struct co_user_monitor {
	co_manager_handle_t handle;
	co_id_t monitor_id;
} co_user_monitor_t;

extern co_rc_t co_user_monitor_create(co_user_monitor_t **out_mon, 
				      co_manager_ioctl_create_t *params);
extern co_rc_t co_user_monitor_open(co_id_t id, co_user_monitor_t **out_mon);
extern void co_user_monitor_close(co_user_monitor_t *monitor);

extern co_rc_t co_user_monitor_any(co_user_monitor_t *monitor, co_monitor_ioctl_op_t op);


extern co_rc_t co_user_monitor_load_section(co_user_monitor_t *umon, 
					    co_monitor_ioctl_load_section_t *params);
extern co_rc_t co_user_monitor_run(co_user_monitor_t *umon, co_monitor_ioctl_run_t *params,
				   unsigned long in_size, unsigned long out_size);
extern co_rc_t co_user_monitor_start(co_user_monitor_t *umon);

extern co_rc_t co_user_monitor_status(co_user_monitor_t *umon, 
				      co_monitor_ioctl_status_t *status);

extern co_rc_t co_user_monitor_destroy(co_user_monitor_t *umon);

#endif
