/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <linux/kernel.h>

#include <memory.h>

#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/monitor.h>
#include <colinux/os/kernel/mutex.h>

#include "manager.h"
#include "monitor.h"

co_rc_t co_manager_init(co_manager_t *manager, void *io_buffer)
{
	co_manager_ioctl_init_t *params;

	params = (typeof(params))(io_buffer);

	manager->host_memory_amount = params->physical_memory_size;
	manager->host_memory_pages = manager->host_memory_amount >> PAGE_SHIFT;

	manager->state = CO_MANAGER_STATE_INITIALIZED;

	return CO_RC(OK);
}

co_rc_t co_manager_cleanup(void **private_data)
{
	co_monitor_t *cmon = NULL;

	cmon = (typeof(cmon))(*private_data);
	if (cmon != NULL) {
		co_debug("Process exited abnormally, cleaning up Monitor\n");

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
			co_debug("invalid ioctl when not initialized\n");
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
			co_debug("monitor ioctl too small! (%d < %d)\n", in_size, sizeof(*params));
			params->rc = CO_RC(CMON_NOT_LOADED);
			break;
		}
		
		cmon = (typeof(cmon))(*private_data);
		if (!cmon) {
			co_debug("No Monitor is attached\n");
			params->rc = CO_RC(CMON_NOT_LOADED);
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
