/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __CO_KERNEL_CONSOLE_H__
#define __CO_KERNEL_CONSOLE_H__

#include "monitor.h"

extern co_rc_t co_monitor_console_send_near(co_monitor_t *cmon, unsigned long *params);
extern co_rc_t co_monitor_console_send_far(co_monitor_t *cmon, 
					   co_monitor_ioctl_console_messages_t *params, 
					   unsigned long out_size,
					   unsigned long *return_size);

#endif
