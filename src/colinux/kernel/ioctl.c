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

#include "manager.h"

#include <colinux/common/ioctl.h>
#include <colinux/os/kernel/monitor.h>
#include <colinux/os/alloc.h>

#include "devices.h"
#include "keyboard.h"
#include "network.h"
#include "console.h"

co_rc_t co_monitor_ioctl(co_monitor_t *cmon, co_manager_ioctl_monitor_t *io_buffer,
			 unsigned long in_size, unsigned long out_size,
			 unsigned long *return_size)
{
	co_rc_t rc = CO_RC_ERROR;

	switch (io_buffer->op) {
	case CO_MONITOR_IOCTL_DESTROY: {
		if (cmon->refcount < 2) {
			co_debug("coLinux destroy: refcount is %d\n", cmon->refcount);
			break;
		}
		
		co_manager_put_monitor(cmon);
		return CO_RC_OK;
	}
	case CO_MONITOR_IOCTL_LOAD_SECTION: {
		return co_monitor_load_section(cmon, 
					       (co_monitor_ioctl_load_section_t *)io_buffer);
	}
	case CO_MONITOR_IOCTL_ATTACH_CONSOLE: {
		co_monitor_ioctl_attach_console_t *params;
		params = (typeof(params))(io_buffer);

		if (out_size < sizeof(*params)) {
			rc = CO_RC(ERROR);
			co_debug("not enough space in the output buffer (%d < %d)\n",
				 out_size, sizeof(*params));
			return rc;
		}

		if (cmon->state != CO_MONITOR_STATE_RUNNING) {
			co_debug("console tried to attached in a bad state (%d)\n",
				 cmon->state);
			return rc;
		}
		
		if (cmon->console_state == CO_MONITOR_CONSOLE_STATE_ATTACHED) {
			rc = CO_RC(ERROR);
			co_debug("console is already attached!\n");
			return rc;
		}

		*return_size = sizeof(*params);
		params->size = cmon->console->size;

		co_monitor_os_clear_console_detach_event(cmon);
		cmon->console_state = CO_MONITOR_CONSOLE_STATE_ATTACHED;
		return CO_RC_OK;
	}
	case CO_MONITOR_IOCTL_DOWNLOAD_CONSOLE: {
		co_console_t *console;
		if (cmon->console_state == CO_MONITOR_CONSOLE_STATE_DETACHED) {
			rc = CO_RC(ERROR);
			co_debug("console is detached!\n");
			return rc;
		}

		if (cmon->console == NULL) {
			rc = CO_RC(ERROR);
			co_debug("console was already downloaded!\n");
			return rc;
		}

		console = cmon->console;

		*return_size += console->size;

		if (*return_size > out_size) {
			rc = CO_RC(ERROR);
			co_debug("not enough space in the output buffer\n");
			return rc;
		}

		cmon->console = NULL;

		co_console_pickle(console);

		memcpy(io_buffer->extra_data, console, console->size); 
		co_console_unpickle(console);
		co_console_destroy(console);
		
		return CO_RC_OK;
	}
	case CO_MONITOR_IOCTL_UPLOAD_CONSOLE: {
		co_console_t *console, *io_console;

		if (cmon->console_state == CO_MONITOR_CONSOLE_STATE_DETACHED) {
			rc = CO_RC(ERROR);
			co_debug("console is attached!\n");
			return rc;
		}

		if (cmon->console != NULL) {
			rc = CO_RC(ERROR);
			co_debug("console was already uploaded!\n");
			return rc;
		}

		io_console = (co_console_t *)io_buffer->extra_data;
		if (io_console->size != in_size) {
			rc = CO_RC(ERROR);
			co_debug("console upload size mismatch (%d != %d)!\n",
				 io_console->size, in_size);
			return rc;
		}

		console = (co_console_t *)co_os_malloc(in_size);
		if (!console) {
			rc = CO_RC(ERROR);
			return rc;
		}

		memcpy(console, io_console, in_size);

		co_console_unpickle(console);

		cmon->console = console;

		return CO_RC_OK;
	}
	case CO_MONITOR_IOCTL_DETACH_CONSOLE: {
		if (cmon->console_state == CO_MONITOR_CONSOLE_STATE_DETACHED) {
			rc = CO_RC(ERROR);
			co_debug("console is already detached!\n");
			return rc;
		}

		if (cmon->console == NULL) {
			rc = CO_RC(ERROR);
			co_debug("console was not uploaded!\n");
			return rc;
		}

		cmon->console_state = CO_MONITOR_CONSOLE_STATE_DETACHED;
		co_monitor_os_set_console_detach_event(cmon);
		return CO_RC_OK;
	}
	case CO_MONITOR_IOCTL_CONSOLE_POLL: {
		co_monitor_os_console_poll(cmon);

		if (cmon->console_poll_cancel) {
			cmon->console_poll_cancel--;
			return CO_RC(ERROR);
		}
		return CO_RC_OK;
	}
	case CO_MONITOR_IOCTL_CONSOLE_CANCEL_POLL: {
		co_os_mutex_acquire(cmon->console_mutex);
		co_queue_flush(&cmon->console_queue);
		co_os_mutex_release(cmon->console_mutex);
		cmon->console_poll_cancel += 1;

		co_monitor_os_console_event(cmon);

		return CO_RC_OK;
	}
	case CO_MONITOR_IOCTL_CONSOLE_MESSAGES: {
		co_monitor_ioctl_console_messages_t *params;

		params = (typeof(params))(io_buffer);

		if (out_size < sizeof(*params)) {
			co_debug("invalid ioctl struct size (%d < %d)",
				 out_size, sizeof(*params));
			return CO_RC(ERROR);
		}

		rc = co_monitor_console_send_far(cmon, params, out_size, return_size);

		return rc;
	}
	case CO_MONITOR_IOCTL_KEYBOARD: {
		co_monitor_ioctl_keyboard_t *params;

		params = (typeof(params))(io_buffer);
		
		if (cmon->state == CO_MONITOR_STATE_RUNNING) { 
			co_monitor_keyboard_input(cmon, params);
			return CO_RC(OK);
		}
		return CO_RC(OK);
	}
	case CO_MONITOR_IOCTL_RUN: {
		if (cmon->state == CO_MONITOR_STATE_INITIALIZED) 
			rc = co_monitor_run(cmon);
		else {
			rc = CO_RC(ERROR);
			co_debug("Invalid state to run (%d)\n", rc);
		}
			
		return rc;
	}
	case CO_MONITOR_IOCTL_STATUS: {
		rc = CO_RC_OK;
			
		return rc;
	}
	case CO_MONITOR_IOCTL_TERMINATE: {
		rc = CO_RC_OK;
		if (cmon->state == CO_MONITOR_STATE_RUNNING) { 
			co_monitor_terminate(cmon);
		}
		return rc;
	}
	case CO_MONITOR_IOCTL_NETWORK_PACKET_POLL: {
		rc = CO_RC_OK;
		co_monitor_os_network_poll(cmon);
		return rc;
	}
	case CO_MONITOR_IOCTL_NETWORK_PACKET_CANCEL_POLL: {
		rc = CO_RC_OK;
		co_monitor_os_network_event(cmon);
		return rc;
	}
	case CO_MONITOR_IOCTL_NETWORK_PACKET_RECEIVE: {
		co_monitor_device_t *device;
		co_monitor_ioctl_network_receive_t *params;

		params = (typeof(params))(io_buffer);

		device = co_monitor_get_device(cmon, CO_DEVICE_NETWORK);

		rc = co_monitor_network_send_far(cmon, device, (char *)io_buffer, out_size);

		if (CO_OK(rc))
			*return_size = sizeof(*params) + params->size;

		co_monitor_put_device(cmon, device);

		return rc;
	}
	case CO_MONITOR_IOCTL_NETWORK_PACKET_SEND: {
		co_monitor_device_t *device;
		
		device = co_monitor_get_device(cmon, CO_DEVICE_NETWORK);

		rc = co_monitor_network_receive_far(cmon, device, 
						    (char *)&io_buffer->extra_data, 
						    in_size);

		co_monitor_put_device(cmon, device);

		return rc;
	}
	case CO_MONITOR_IOCTL_SYSDEP: {
		return rc;
	}
	default:
		return rc;
	}

	return rc;
}

co_rc_t co_manager_init(co_manager_t *manager, void *io_buffer)
{
	co_manager_ioctl_init_t *params;

	params = (typeof(params))(io_buffer);

	manager->host_memory_amount = params->physical_memory_size;
	manager->host_memory_pages = manager->host_memory_amount >> PAGE_SHIFT;

	manager->state = CO_MANAGER_STATE_INITIALIZED;

	return CO_RC(OK);
}

co_rc_t co_manager_ioctl(co_manager_t *manager, co_monitor_ioctl_op_t ioctl, 
			 void *io_buffer, unsigned long in_size,
			 unsigned long out_size, unsigned long *return_size)
{
	co_rc_t rc = CO_RC_OK;

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
		co_manager_ioctl_create_t *params;
		co_monitor_t *cmon = NULL;

		params = (typeof(params))(io_buffer);
		params->rc = co_manager_create_monitor(manager, &cmon);

		*return_size = sizeof(*params);

		if (CO_OK(params->rc)) {
			co_monitor_init(cmon, params);

			params->new_id = cmon->id;
		}

		return rc;
	}
	case CO_MANAGER_IOCTL_MONITOR: {
		co_manager_ioctl_monitor_t *params;
		co_monitor_t *cmon = NULL;

		params = (typeof(params))(io_buffer);
		*return_size = sizeof(*params);

		if (in_size < sizeof(*params)) {
			co_debug("monitor ioctl too small! (%d < %d)\n", 
				 in_size, sizeof(*params));
			params->rc = CO_RC(CMON_NOT_LOADED);
			break;
		}
		
		rc = co_manager_get_monitor(manager, params->id, &cmon);
		if (CO_OK(rc)) {
			in_size -= sizeof(*params);
			params->rc = co_monitor_ioctl(cmon, params, 
						      in_size, out_size, 
						      return_size);
			co_manager_put_monitor(cmon);
			break;
		} 

		params->rc = CO_RC(CMON_NOT_LOADED);
		break;

	}
	default:
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}
