/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <colinux/os/user/misc.h>

#include "manager.h"

co_rc_t co_manager_io_monitor(co_manager_handle_t handle, 
			       co_monitor_ioctl_op_t op,
			       co_manager_ioctl_monitor_t *ioctl,
			       unsigned long in_size,
			       unsigned long out_size)
{
	unsigned long returned = 0;
	co_rc_t rc;

	ioctl->op = op;
	ioctl->rc = CO_RC_OK;
	
	rc = co_os_manager_ioctl(handle, CO_MANAGER_IOCTL_MONITOR,
				 ioctl, in_size, ioctl, out_size, &returned);

	return ioctl->rc;
}

co_rc_t co_manager_io_monitor_unisize(co_manager_handle_t handle,
				      co_monitor_ioctl_op_t op,
				      co_manager_ioctl_monitor_t *ioctl,
				      unsigned long size)
{
	return co_manager_io_monitor(handle, op, ioctl, size, size);
}

co_rc_t co_manager_status(co_manager_handle_t handle, co_manager_ioctl_status_t *status)
{
	co_rc_t rc;
	unsigned long returned = 0;

	rc = co_os_manager_ioctl(handle, CO_MANAGER_IOCTL_STATUS,
				 status, sizeof(*status), status, sizeof(*status), &returned);

	if (status->periphery_api_version != CO_LINUX_PERIPHERY_API_VERSION) {
		co_terminal_print("colinux: driver version mismatch: expected %d got %d\n",
				  CO_LINUX_PERIPHERY_API_VERSION, status->periphery_api_version);
		return CO_RC(VERSION_MISMATCHED);
	}

	return rc;
}
