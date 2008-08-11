#include <colinux/common/ioctl.h>
#include <colinux/os/user/manager.h>

#include "monitor.h"
#include "manager.h"

/**
 * Returns PID of first monitor.
 *
 * If none found, returns CO_INVALID_ID.
 *
 * TODO: Find first monitor not already attached.
 */
co_id_t find_first_monitor(void)
{
	co_manager_handle_t handle;
	co_manager_ioctl_monitor_list_t	list;
	co_rc_t	rc;

	handle = co_os_manager_open();
	if (handle == NULL)
		return CO_INVALID_ID;

	rc = co_manager_monitor_list(handle, &list);
	co_os_manager_close(handle);
	if (!CO_OK(rc) || list.count == 0)
		return CO_INVALID_ID;

	return list.ids[0];
}

