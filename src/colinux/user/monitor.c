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

co_rc_t co_user_monitor_create(co_user_monitor_t **out_mon, co_manager_ioctl_create_t *params)
{
	co_manager_handle_t handle;
	co_user_monitor_t *mon;

	mon = co_os_malloc(sizeof(*mon));
	if (!mon)
		return CO_RC(ERROR);

	handle = co_os_manager_open();
	if (!handle) {
		co_os_free(mon);
		return CO_RC(ERROR);
	}

	co_os_manager_ioctl(handle, CO_MANAGER_IOCTL_CREATE,
			    params, sizeof(*params), params, sizeof(*params), NULL);

	if (!CO_OK(params->rc)) {
		co_os_free(mon);
		co_os_manager_close(handle);
		return params->rc;
	}
	
	mon->handle = handle;
	*out_mon = mon;

	return CO_RC(OK);
}

co_rc_t co_user_monitor_open(co_id_t id, co_user_monitor_t **out_mon)
{
	co_user_monitor_t *mon;
	co_manager_handle_t handle;

	mon = co_os_malloc(sizeof(*mon));
	if (!mon)
		return CO_RC(ERROR);

	handle = co_os_manager_open();
	if (!handle) {
		co_os_free(mon);
		return CO_RC(ERROR);
	}

	mon->monitor_id = id;
	mon->handle = handle;

	*out_mon = mon;

	return CO_RC(OK);
}

void co_user_monitor_close(co_user_monitor_t *monitor)
{
	co_os_manager_close(monitor->handle);
	co_os_free(monitor);
}

co_rc_t co_user_monitor_load_section(co_user_monitor_t *umon, 
				     co_monitor_ioctl_load_section_t *params)
{
	co_monitor_ioctl_load_section_t *params_copy = NULL;
	unsigned long alloc_size = 0;
	co_rc_t rc;

	alloc_size = sizeof(*params);
	if (params->user_ptr) 
		alloc_size += params->size;
    
	params_copy = (co_monitor_ioctl_load_section_t *)co_os_malloc(alloc_size);
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
    
	params_copy = (co_monitor_ioctl_load_initrd_t *)co_os_malloc(alloc_size);
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

co_rc_t co_user_monitor_run(co_user_monitor_t *umon, co_monitor_ioctl_run_t *params,
			    unsigned long in_size, unsigned long out_size)
{
	return co_manager_io_monitor(umon->handle,
				     CO_MONITOR_IOCTL_RUN, &params->pc,
				     in_size, out_size);
}

co_rc_t co_user_monitor_start(co_user_monitor_t *umon)
{
	return co_manager_io_monitor_simple(umon->handle,  CO_MONITOR_IOCTL_START);
}

co_rc_t co_user_monitor_any(co_user_monitor_t *umon, co_monitor_ioctl_op_t op)
{
	return co_manager_io_monitor_simple(umon->handle, op); 
}

co_rc_t co_user_monitor_status(co_user_monitor_t *umon, 
			       co_monitor_ioctl_status_t *params)
{
	return co_manager_io_monitor_unisize(umon->handle, 
					     CO_MONITOR_IOCTL_STATUS, 
					     &params->pc, sizeof(*params));
}

co_rc_t co_user_monitor_destroy(co_user_monitor_t *umon)
{
	return co_manager_io_monitor_simple(umon->handle, 
					    CO_MONITOR_IOCTL_DESTROY);
}
