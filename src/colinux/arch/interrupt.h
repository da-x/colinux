/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_ARCH_INTERRUPT_H__
#define __COLINUX_ARCH_INTERRUPT_H__

#include <colinux/kernel/monitor.h>

extern void co_monitor_arch_real_hardware_interrupt(co_monitor_t *cmon);

#endif
