/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_KEYBOARD_H__
#define __COLINUX_KERNEL_KEYBOARD_H__

#include "monitor.h"
#include <colinux/common/ioctl.h>

extern co_rc_t co_monitor_keyboard_input(co_monitor_t *cmon, 
					 co_monitor_ioctl_keyboard_t *params);
extern co_rc_t co_monitor_service_keyboard(co_monitor_t *cmon, 
					   co_monitor_device_t *device,
					   unsigned long *params);

#endif

