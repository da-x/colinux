/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __CO_KERNEL_NETWORK_H__
#define __CO_KERNEL_NETWORK_H__

#include "monitor.h"
#include "devices.h"

extern co_rc_t co_monitor_network_send_near(co_monitor_t *cmon, 
					    co_monitor_device_t *device,
					    unsigned long *params);

extern co_rc_t co_monitor_network_send_far(co_monitor_t *cmon, 
					   co_monitor_device_t *device,
					   unsigned char *buf,
					   unsigned long size);

extern co_rc_t co_monitor_network_receive_far(co_monitor_t *cmon, 
					      co_monitor_device_t *device,
					      unsigned char *buf, 
					      unsigned long size);

extern co_rc_t co_monitor_network_receive_near(co_monitor_t *cmon, 
					       co_monitor_device_t *device,
					       unsigned long *params);


#endif
