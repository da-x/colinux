/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __COLINUX_USER_MANAGER_H__
#define __COLINUX_USER_MANAGER_H__

#include <colinux/common/ioctl.h>
#include <colinux/os/user/manager.h>

extern co_rc_t co_manager_io_monitor(co_manager_handle_t handle, co_id_t id, 
					co_monitor_ioctl_op_t op,
					co_manager_ioctl_monitor_t *ioctl,
					unsigned long in_size,
					unsigned long out_size);

extern co_rc_t co_manager_io_monitor_unisize(co_manager_handle_t handle, co_id_t id, 
					     co_monitor_ioctl_op_t op,
					     co_manager_ioctl_monitor_t *ioctl,
					     unsigned long size);

extern co_rc_t co_manager_init(co_manager_handle_t handle);

#endif
