/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <linux/kernel.h>
#include <colinux/os/kernel/monitor.h>

#include "console.h"

co_rc_t co_monitor_console_send_near(co_monitor_t *cmon, unsigned long *params)
{
	co_console_message_t *message;
	co_monitor_ioctl_console_message_t *ioctl_message = NULL;
	unsigned long ioctl_message_size;
	co_rc_t rc = CO_RC(OK);

	message = (typeof(message))(params);

	co_os_mutex_acquire(cmon->console_mutex);

	if (cmon->console_state == CO_MONITOR_CONSOLE_STATE_DETACHED) {
		rc = co_console_op(cmon->console, message);
		goto out; 
	}
	
	ioctl_message_size = (((char *)&ioctl_message->extra_data) - (char *)ioctl_message);
	ioctl_message_size += message->size;

	rc = co_queue_malloc(&cmon->console_queue, ioctl_message_size, 
			     (void **)&ioctl_message);
	if (!CO_OK(rc))
		goto out;

	ioctl_message->type = CO_MONITOR_IOCTL_CONSOLE_MESSAGE_NORMAL;
	ioctl_message->size = ioctl_message_size;
	memcpy(ioctl_message->extra_data, message, message->size);

	co_queue_add_head(&cmon->console_queue, ioctl_message);

out:
	co_os_mutex_release(cmon->console_mutex);

	co_monitor_os_console_event(cmon);

	return rc;
}

co_rc_t co_monitor_console_send_far(co_monitor_t *cmon, 
				    co_monitor_ioctl_console_messages_t *params, 
				    unsigned long out_size,
				    unsigned long *return_size)
{
	unsigned long queue_items, size_left;
	char *params_data;
	co_rc_t rc = CO_RC_OK;

	params->num_messages = 0;
	params_data = params->data;
	size_left = out_size - sizeof(*params);

	co_os_mutex_acquire(cmon->console_mutex);
	queue_items = co_queue_size(&cmon->console_queue);
	while (queue_items > 0) {
		co_monitor_ioctl_console_message_t *message;
			
		rc = co_queue_pop_tail(&cmon->console_queue, (void **)&message);
		if (!CO_OK(rc))
			break;

		if (message->size > size_left) {
			if (params->num_messages != 0)
				co_queue_add_tail(&cmon->console_queue, message);
			else
				co_queue_free(&cmon->console_queue, message);
			break;
		}				
			
		memcpy(params_data, message, message->size);

		queue_items -= 1;
		size_left -= message->size;
		params_data += message->size;
		params->num_messages += 1;

		co_queue_free(&cmon->console_queue, message);
	}

	*return_size = (char *)params_data - (char *)params;

	co_os_mutex_release(cmon->console_mutex);

	return rc;
}
