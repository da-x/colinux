/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_DEVICES_H__
#define __COLINUX_KERNEL_DEVICES_H__

#include "monitor.h"

extern co_rc_t co_monitor_signal_device(co_monitor_t *cmon, co_device_t device);
extern co_monitor_device_t *co_monitor_get_device(co_monitor_t *cmon, co_device_t device);
extern void co_monitor_put_device(co_monitor_t *cmon, co_monitor_device_t *intr);

extern void co_monitor_check_devices(co_monitor_t *cmon);
extern co_rc_t co_monitor_setup_devices(co_monitor_t *cmon);
extern co_rc_t co_monitor_cleanup_devices(co_monitor_t *cmon);

#endif
