/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "monitor.h"

#include "keyboard.h"
#include "network.h"

#include <colinux/os/kernel/monitor.h>

void co_monitor_check_devices(co_monitor_t *cmon)
{
	int device_id;

	for (device_id=0; device_id < CO_DEVICES_TOTAL; device_id++) {
		co_monitor_device_t *device = &cmon->devices[device_id];
		co_rc_t rc = CO_RC(OK);

		if (!device->state)
			continue;

		device->state = 0;

		if (device->service) {
			rc = device->service(cmon, device, &co_passage_page->params[2]);
		}

		if (CO_OK(rc)) {
 			/* co_passage_page->operation = CO_OPERATION_BACKWARD_INTERRUPT; */
			co_passage_page->params[0] = device_id;
			co_passage_page->params[1] = device->state;
			
			co_passage_page->colx_state.flags &= ~(1 << 9); /* Turn IF off */
			break;
		}
	}
}

co_rc_t co_monitor_setup_devices(co_monitor_t *cmon)
{
	int i = 0;
	co_monitor_service_func_t funcs[CO_DEVICES_TOTAL] = {
		[CO_DEVICE_KEYBOARD] = co_monitor_service_keyboard,
		[CO_DEVICE_NETWORK] = co_monitor_network_receive_near,
		};
	co_rc_t rc;

	for (i=0; i < CO_DEVICES_TOTAL; i++) {
		cmon->devices[i].service = funcs[i];
		co_queue_init(&cmon->devices[i].in_queue);
		co_queue_init(&cmon->devices[i].out_queue);
		rc = co_os_mutex_create(&cmon->devices[i].mutex);
	}

	return CO_RC(OK);
}

co_rc_t co_monitor_cleanup_devices(co_monitor_t *cmon)
{
	int i = 0;

	for (i=0; i < CO_DEVICES_TOTAL; i++) {
		co_queue_flush(&cmon->devices[i].in_queue);
		co_queue_flush(&cmon->devices[i].out_queue);
		co_os_mutex_destroy(cmon->devices[i].mutex);
	}

	return CO_RC(OK);
}

co_rc_t co_monitor_signal_device(co_monitor_t *cmon, co_device_t device)
{
	cmon->devices[device].state = 1;

 	co_monitor_os_wakeup(cmon);

	return CO_RC(OK);
}

co_monitor_device_t *co_monitor_get_device(co_monitor_t *cmon, co_device_t device)
{
	return &cmon->devices[device];
}

void co_monitor_put_device(co_monitor_t *cmon, co_monitor_device_t *intr)
{
}

