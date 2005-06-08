/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_ARCH_PASSAGE_H__
#define __COLINUX_ARCH_PASSAGE_H__

#include <colinux/kernel/monitor.h>

extern co_rc_t co_monitor_arch_passage_page_alloc(co_monitor_t *cmon);
extern co_rc_t co_monitor_arch_passage_page_init(co_monitor_t *cmon);
extern void co_monitor_arch_passage_page_free(co_monitor_t *cmon);
extern void co_host_switch_wrapper(co_monitor_t *cmon);
extern void co_monitor_arch_enable_interrupts(void);

#endif
