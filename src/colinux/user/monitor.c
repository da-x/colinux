/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <memory.h>

#include <colinux/common/ioctl.h>
#include <colinux/os/alloc.h>
#include <colinux/os/user/manager.h>

#include "monitor.h"
#include "manager.h"

co_rc_t co_manager_io_monitor_simple(co_manager_handle_t handle, co_monitor_ioctl_op_t op)
{
	co_manager_ioctl_monitor_t ioctl;

	return co_manager_io_monitor_unisize(handle, op, &ioctl, sizeof(ioctl));
}

co_rc_t co_user_monitor_create(co_user_monitor_t**        out_mon,
			       co_manager_ioctl_create_t* params,
			       co_manager_handle_t 	  handle)
{
	co_user_monitor_t *mon;
	co_rc_t rc;

	mon = co_os_malloc(sizeof(*mon));
	if (!mon)
		return CO_RC(OUT_OF_MEMORY);

	memset(mon, 0, sizeof(*mon));

	rc = co_os_manager_ioctl(handle,
				 CO_MANAGER_IOCTL_CREATE,
				 params,
				 sizeof(*params),
				 params,
				 sizeof(*params),
				 NULL);

	if (!CO_OK(rc)) {
		co_os_free(mon);
		return rc;
	}

	if (!CO_OK(params->rc)) {
		co_os_free(mon);
		return params->rc;
	}

	mon->handle = handle;
	*out_mon = mon;

	return CO_RC(OK);
}

co_rc_t co_user_monitor_open(co_reactor_t 		    reactor,
			     co_reactor_user_receive_func_t receive,
			     co_id_t			    id,
			     co_module_t*		    modules,
			     int			    num_modules,
			     co_user_monitor_t**	    out_mon)
{
	co_user_monitor_t*	  mon;
	co_manager_handle_t	  handle;
	co_manager_ioctl_attach_t params;
	co_rc_t 		  rc;
	int			  modules_copied = 0;

	mon = co_os_malloc(sizeof(*mon));
	if (!mon)
		return CO_RC(OUT_OF_MEMORY);

	memset(mon, 0, sizeof(*mon));

	handle = co_os_manager_open();
	if (!handle) {
		co_os_free(mon);
		return CO_RC(ERROR_ACCESSING_DRIVER);
	}

	params.id = id;
	for (modules_copied = 0;
	     modules_copied < num_modules && modules_copied < CO_MANAGER_ATTACH_MAX_MODULES;
	     modules_copied++)
	{
		params.modules[modules_copied] = modules[modules_copied];
	}
	params.num_modules = num_modules;

	rc = co_manager_attach(handle, &params);

	if (!CO_OK(rc)) {
		co_os_manager_close(handle);
		co_os_free(mon);
		return rc;
	}

	rc = co_os_reactor_monitor_create(
		reactor, handle,
		receive, &mon->reactor_user);

	if (!CO_OK(rc)) {
		co_os_manager_close(handle);
		co_os_free(mon);
		return rc;
	}

	mon->monitor_id = id;
	mon->handle     = handle;

	*out_mon = mon;

	return rc;
}

void co_user_monitor_close(co_user_monitor_t *monitor)
{
	if (monitor->reactor_user) {
		co_os_reactor_monitor_destroy(monitor->reactor_user);
	}

	co_manager_io_monitor_simple(monitor->handle, CO_MONITOR_IOCTL_CLOSE);
	co_os_manager_close(monitor->handle);
	co_os_free(monitor);
}

co_rc_t co_user_monitor_load_section(co_user_monitor_t*		      umon,
				     co_monitor_ioctl_load_section_t* params)
{
	co_monitor_ioctl_load_section_t *params_copy = NULL;
	unsigned long alloc_size = 0;
	co_rc_t rc;

	alloc_size = sizeof(*params);
	if (params->user_ptr)
		alloc_size += params->size;

	params_copy = co_os_malloc(alloc_size);
	if (!params_copy)
		return CO_RC(OUT_OF_MEMORY);

	*params_copy = *params;

	if (params->user_ptr)
		memcpy((unsigned char *)params_copy + sizeof(*params),
		       params->user_ptr, params->size);

	rc = co_manager_io_monitor_unisize(umon->handle,
					   CO_MONITOR_IOCTL_LOAD_SECTION,
					   &params_copy->pc, alloc_size);

	co_os_free(params_copy);

	return rc;
}

co_rc_t co_user_monitor_load_initrd(co_user_monitor_t *umon,
				    void *initrd, unsigned long initrd_size)
{
	co_monitor_ioctl_load_initrd_t *params_copy = NULL;
	unsigned long alloc_size = 0;
	co_rc_t rc;

	alloc_size = sizeof(*params_copy) + initrd_size;

	params_copy = co_os_malloc(alloc_size);
	if (!params_copy)
		return CO_RC(OUT_OF_MEMORY);

	params_copy->size = initrd_size;
	memcpy(params_copy->buf, initrd, initrd_size);

	rc = co_manager_io_monitor_unisize(umon->handle,
					   CO_MONITOR_IOCTL_LOAD_INITRD,
					   &params_copy->pc, alloc_size);

	co_os_free(params_copy);

	return rc;
}

co_rc_t co_user_monitor_run(co_user_monitor_t *umon, co_monitor_ioctl_run_t *params)
{
	return co_manager_io_monitor(umon->handle,
				     CO_MONITOR_IOCTL_RUN, &params->pc,
				     sizeof(*params), sizeof(*params));
}

co_rc_t co_user_monitor_start(co_user_monitor_t *umon)
{
	return co_manager_io_monitor_simple(umon->handle,  CO_MONITOR_IOCTL_START);
}

co_rc_t co_user_monitor_any(co_user_monitor_t *umon, co_monitor_ioctl_op_t op)
{
	return co_manager_io_monitor_simple(umon->handle, op);
}

/* Get console dimensions and max buffer size */
co_rc_t co_user_monitor_get_console(co_user_monitor_t*              umon,
				    co_monitor_ioctl_get_console_t* params)
{
	return co_manager_io_monitor_unisize(umon->handle,
					     CO_MONITOR_IOCTL_GET_CONSOLE,
					     &params->pc,
					     sizeof(*params));
}

co_rc_t co_user_monitor_get_state(co_user_monitor_t*            umon,
				  co_monitor_ioctl_get_state_t* params)
{
	return co_manager_io_monitor_unisize(umon->handle,
					     CO_MONITOR_IOCTL_GET_STATE,
					     &params->pc,
					     sizeof(*params));
}

co_rc_t co_user_monitor_reset(co_user_monitor_t* umon)
{
	return co_manager_io_monitor_simple(umon->handle, CO_MONITOR_IOCTL_RESET);
}


co_rc_t co_user_monitor_message_send(co_user_monitor_t* umon,  co_message_t* message)
{
	return umon->reactor_user->send(umon->reactor_user,
					(unsigned char*)message,
					message->size  + sizeof(*message));
}

co_rc_t co_user_monitor_status(co_user_monitor_t *umon,
			       co_monitor_ioctl_status_t *params)
{
	return co_manager_io_monitor_unisize(umon->handle,
					     CO_MONITOR_IOCTL_STATUS,
					     &params->pc, sizeof(*params));
}

co_rc_t co_user_monitor_conet_bind_adapter(co_user_monitor_t *umon,
				co_monitor_ioctl_conet_bind_adapter_t *params)
{
	return co_manager_io_monitor_unisize(umon->handle,
					     CO_MONITOR_IOCTL_CONET_BIND_ADAPTER,
					     &params->pc, sizeof(*params));
}

co_rc_t co_user_monitor_conet_unbind_adapter(co_user_monitor_t *umon,
				co_monitor_ioctl_conet_unbind_adapter_t *params)
{
	return co_manager_io_monitor_unisize(umon->handle,
					     CO_MONITOR_IOCTL_CONET_UNBIND_ADAPTER,
					     &params->pc, sizeof(*params));
}

