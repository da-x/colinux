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

#include "monitor.h"
#include "devices.h"

#include <colinux/kernel/transfer.h>
#include <colinux/os/kernel/monitor.h>
#include <colinux/common/queue.h>

typedef struct {
	unsigned long size;
	unsigned char data[];
} co_queued_packet_t;

co_rc_t co_monitor_network_send_near(co_monitor_t *cmon, 
				     co_monitor_device_t *device,
				     unsigned long *params)
{
	co_queued_packet_t *queue_buf;
	unsigned long size;
	unsigned long data;
	co_rc_t rc = CO_RC(OK);

	co_os_mutex_acquire(device->mutex);

	data = params[0];
	size = params[1];


	rc = co_queue_malloc(&device->out_queue, 
			     size + sizeof(co_queued_packet_t),
			     (void **)&queue_buf);
	
	if (CO_OK(rc)) {
		queue_buf->size = size;
		co_monitor_colx_to_host(cmon, data, queue_buf->data, queue_buf->size);
		co_queue_add_head(&device->out_queue, queue_buf);
	}

	co_os_mutex_release(device->mutex);

	co_monitor_os_network_event(cmon);

	return CO_RC(OK);
}

co_rc_t co_monitor_network_send_far(co_monitor_t *cmon, 
				    co_monitor_device_t *device,
				    unsigned char *buf,
				    unsigned long size)
{
	co_queued_packet_t *queue_buf;
	co_rc_t rc = CO_RC_ERROR;

	co_os_mutex_acquire(device->mutex);

	rc = co_queue_pop_tail(&device->out_queue, (void **)&queue_buf);

	if (CO_OK(rc)) {
		co_monitor_ioctl_network_receive_t *params;
		params = (typeof(params))(buf);

		size -= sizeof(*params);

		if (queue_buf->size <= size) {
			params->size = queue_buf->size;
			memcpy(params->data, queue_buf->data, queue_buf->size);
		}
		else {
			params->size = 0;
		}

		co_queue_free(&device->out_queue, queue_buf);

		params->more = co_queue_size(&device->out_queue);
	}

	co_os_mutex_release(device->mutex);

	return rc;
}

co_rc_t co_monitor_network_receive_far(co_monitor_t *cmon, 
				       co_monitor_device_t *device,
				       unsigned char *buf, 
				       unsigned long size)
{
	co_rc_t rc;

	co_os_mutex_acquire(device->mutex);

	if (size > 0) {
		co_queued_packet_t *queue_buf;

		rc = co_queue_malloc(&device->in_queue, 
				     size + sizeof(co_queued_packet_t),
				     (void **)&queue_buf);
	
		if (CO_OK(rc)) {
			queue_buf->size = size;
			memcpy(queue_buf->data, buf, size);

			co_queue_add_head(&device->in_queue, queue_buf);
		}
	}

	co_monitor_signal_device(cmon, CO_DEVICE_NETWORK);

	co_os_mutex_release(device->mutex);

	return CO_RC(OK);
}

co_rc_t co_monitor_network_receive_near(co_monitor_t *cmon, 
					co_monitor_device_t *device,
					unsigned long *params)
{
	co_queued_packet_t *queue_buf;
	co_rc_t rc;

	co_os_mutex_acquire(device->mutex);

	rc = co_queue_pop_tail(&device->in_queue, (void **)&queue_buf);

	if (CO_OK(rc)) {
		params[0] = queue_buf->size;
		memcpy(&params[1], queue_buf->data, queue_buf->size);
		co_queue_free(&device->in_queue, queue_buf);
		
		if (co_queue_size(&device->in_queue) != 0)
			device->state = 1;
	}

	co_os_mutex_release(device->mutex);
		
	return CO_RC(OK);
}
