/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_KERNEL_MONITOR_H__
#define __COLINUX_OS_KERNEL_MONITOR_H__

#include <colinux/common/ioctl.h>
#include <colinux/kernel/monitor.h>

extern void co_monitor_os_console_poll(co_monitor_t *cmon);
extern void co_monitor_os_console_event(co_monitor_t *cmon);

extern void co_monitor_os_network_poll(co_monitor_t *cmon);
extern void co_monitor_os_network_event(co_monitor_t *cmon);

extern void co_monitor_os_clear_console_detach_event(co_monitor_t *cmon);
extern void co_monitor_os_set_console_detach_event(co_monitor_t *cmon);
extern void co_monitor_os_wait_console_detach(co_monitor_t *cmon);

extern void co_monitor_os_idle(co_monitor_t *cmon);
extern void co_monitor_os_wakeup(co_monitor_t *cmon);

extern co_rc_t co_monitor_os_init(co_monitor_t *cmon);
extern void co_monitor_os_exit(co_monitor_t *cmon);

extern void co_monitor_os_exclusive(co_monitor_t *cmon);

#endif
