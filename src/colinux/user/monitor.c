/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <linux/kernel.h>

#include <memory.h>

#include <colinux/common/ioctl.h>
#include <colinux/os/alloc.h>
#include <colinux/os/user/manager.h>

#include "monitor.h"
#include "manager.h"

co_rc_t co_manager_io_monitor_simple(co_manager_handle_t handle, co_id_t id, 
				     co_monitor_ioctl_op_t op)
{
	co_manager_ioctl_monitor_t ioctl;

	return co_manager_io_monitor_unisize(handle, id, op, &ioctl, sizeof(ioctl));
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
	
	mon->monitor_id = params->new_id;
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
	co_os_manager_close(monitor->handle);;
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

	rc = co_manager_io_monitor_unisize(umon->handle, umon->monitor_id,
					   CO_MONITOR_IOCTL_LOAD_SECTION, 
					   &params_copy->pc, alloc_size);
	
	co_os_free(params_copy);

	return rc;
}

co_rc_t co_user_monitor_run(co_user_monitor_t *umon)
{
	return co_manager_io_monitor_simple(umon->handle, umon->monitor_id, 
					    CO_MONITOR_IOCTL_RUN);
}

co_rc_t co_user_monitor_get_console(co_user_monitor_t *umon, co_console_t **console_out)
{
	co_rc_t rc;
	co_monitor_ioctl_attach_console_t params;
	co_manager_ioctl_monitor_t *download_buffer;
	unsigned long download_size;
	co_console_t *console;

	rc = co_manager_io_monitor_unisize(umon->handle, umon->monitor_id, 
					   CO_MONITOR_IOCTL_ATTACH_CONSOLE, 
					   &params.pc, sizeof(params));

	if (!CO_OK(rc))
		return rc;
	
	download_size = sizeof(*download_buffer) + params.size;
	download_buffer = (co_manager_ioctl_monitor_t *)(co_os_malloc(download_size));

	if (!download_buffer) {
		rc = CO_RC(ERROR);
		goto out_err;
	}

	rc = co_manager_io_monitor(umon->handle, umon->monitor_id,
				   CO_MONITOR_IOCTL_DOWNLOAD_CONSOLE, 
				   download_buffer, sizeof(*download_buffer), 
				   download_size);

	console = (co_console_t *)co_os_malloc(params.size);
	if (!console) {
		rc = CO_RC(ERROR);
		goto out_free_download_buffer;
	}

	memcpy(console, download_buffer->extra_data, params.size);
	co_os_free(download_buffer);

	co_console_unpickle(console);
	*console_out = console;

	rc = CO_RC(OK);
	return rc;

out_free_download_buffer:
	co_os_free(download_buffer);

out_err:
	return rc;
}

co_rc_t co_user_monitor_put_console(co_user_monitor_t *umon, co_console_t *console)
{
	co_rc_t rc;
	co_manager_ioctl_monitor_t *upload_buffer;
	unsigned long upload_size;

	upload_size = sizeof(*upload_buffer) + console->size;
	upload_buffer = (co_manager_ioctl_monitor_t *)(co_os_malloc(upload_size));
	if (!upload_buffer) {
		rc = CO_RC(ERROR);
		goto out_err;
	}

	co_console_pickle(console);
	memcpy(upload_buffer->extra_data, console, console->size);
	co_console_unpickle(console);

	rc = co_manager_io_monitor(umon->handle, umon->monitor_id,
				   CO_MONITOR_IOCTL_UPLOAD_CONSOLE, 
				   upload_buffer, upload_size, 
				   sizeof(*upload_buffer));
	if (!CO_OK(rc))
		goto out_free;

	co_console_destroy(console);
	
	co_manager_io_monitor_simple(umon->handle, umon->monitor_id,
				     CO_MONITOR_IOCTL_DETACH_CONSOLE);

out_free:
	co_os_free(upload_buffer);

out_err:
	return rc;
}

co_rc_t co_user_monitor_any(co_user_monitor_t *umon, co_monitor_ioctl_op_t op)
{
	return co_manager_io_monitor_simple(umon->handle, umon->monitor_id, op); 
}

co_rc_t co_user_monitor_network_send(co_user_monitor_t *umon, 
				     char *data, unsigned long size)
{
	return co_manager_io_monitor(umon->handle, umon->monitor_id, 
				     CO_MONITOR_IOCTL_NETWORK_PACKET_SEND, (co_manager_ioctl_monitor_t *)data,
				     size, sizeof(co_manager_ioctl_monitor_t));
}


co_rc_t co_user_monitor_network_receive(co_user_monitor_t *umon, 
					char *data, unsigned long size)
{
	return co_manager_io_monitor(umon->handle, umon->monitor_id, 
				     CO_MONITOR_IOCTL_NETWORK_PACKET_RECEIVE, (co_manager_ioctl_monitor_t *)data,
				     sizeof(co_manager_ioctl_monitor_t), size);
}

co_rc_t co_user_monitor_network_poll(co_user_monitor_t *umon)
{
	return co_manager_io_monitor_simple(umon->handle, umon->monitor_id, 
					    CO_MONITOR_IOCTL_NETWORK_PACKET_POLL);
}

co_rc_t co_user_monitor_network_cancel_poll(co_user_monitor_t *umon)
{
	return co_manager_io_monitor_simple(umon->handle, umon->monitor_id, 
					    CO_MONITOR_IOCTL_NETWORK_PACKET_CANCEL_POLL);
}

co_rc_t co_user_monitor_console_messages(co_user_monitor_t *umon, 
					 co_monitor_ioctl_console_messages_t *params,
					 unsigned long size)
{
	return co_manager_io_monitor(umon->handle, umon->monitor_id, 
				     CO_MONITOR_IOCTL_CONSOLE_MESSAGES, &params->pc,
				     sizeof(params->pc), size);
}

co_rc_t co_user_monitor_console_poll(co_user_monitor_t *umon)
{
	return co_manager_io_monitor_simple(umon->handle, umon->monitor_id, 
					    CO_MONITOR_IOCTL_CONSOLE_POLL);
}

co_rc_t co_user_monitor_console_cancel_poll(co_user_monitor_t *umon)
{
	return co_manager_io_monitor_simple(umon->handle, umon->monitor_id, 
					    CO_MONITOR_IOCTL_CONSOLE_CANCEL_POLL);
}

co_rc_t co_user_monitor_keyboard(co_user_monitor_t *umon, 
				 co_monitor_ioctl_keyboard_t *params)
{
	return co_manager_io_monitor_unisize(umon->handle, umon->monitor_id, 
					     CO_MONITOR_IOCTL_KEYBOARD, 
					     &params->pc, sizeof(*params));
}

co_rc_t co_user_monitor_status(co_user_monitor_t *umon, 
			       co_monitor_ioctl_status_t *params)
{
	return co_manager_io_monitor_unisize(umon->handle, umon->monitor_id, 
					     CO_MONITOR_IOCTL_STATUS, 
					     &params->pc, sizeof(*params));
}

co_rc_t co_user_monitor_terminate(co_user_monitor_t *umon)
{
	return co_manager_io_monitor_simple(umon->handle, umon->monitor_id, 
					    CO_MONITOR_IOCTL_TERMINATE);
}

co_rc_t co_user_monitor_destroy(co_user_monitor_t *umon)
{
	return co_manager_io_monitor_simple(umon->handle, umon->monitor_id, 
					    CO_MONITOR_IOCTL_DESTROY);
}
