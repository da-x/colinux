/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <linux/kernel.h>

#include "keyboard.h"
#include "devices.h"

co_rc_t co_monitor_keyboard_input(co_monitor_t *cmon, co_monitor_ioctl_keyboard_t *params)
{
	switch (params->op) {
	case CO_OPERATION_KEYBOARD_ACTION: {
		co_rc_t rc;
		co_monitor_device_t *intr;
		co_scan_code_t *sc;

		intr = co_monitor_get_device(cmon, CO_DEVICE_KEYBOARD);
		
		KASSERT(intr != NULL);
		
		co_os_mutex_acquire(intr->mutex);
		rc = co_queue_malloc(&intr->in_queue, sizeof(params->sc), (void **)&sc);
		if (CO_OK(rc)) {
			*sc = params->sc;
			co_queue_add_head(&intr->in_queue, sc);
		}
		co_os_mutex_release(intr->mutex);

		co_monitor_signal_device(cmon, CO_DEVICE_KEYBOARD);
		co_monitor_put_device(cmon, intr);

		break;
	}
	default:
		break;
	}

	return CO_RC(OK);
}


co_rc_t co_monitor_service_keyboard(co_monitor_t *cmon, 
				    co_monitor_device_t *device,
				    unsigned long *params)
{
	co_scan_code_t *ptr;
	co_rc_t rc;
	
	co_os_mutex_acquire(device->mutex);

	rc = co_queue_pop_tail(&device->in_queue, (void **)&ptr);
	if (CO_OK(rc)) {
		params[0] = ptr->code;
		params[1] = ptr->down;

		co_queue_free(&device->in_queue, ptr);

		if (co_queue_size(&device->in_queue) != 0)
			device->state = 1;
	}

	co_os_mutex_release(device->mutex);
	
	return CO_RC(OK);
}
