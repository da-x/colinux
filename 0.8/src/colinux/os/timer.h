/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_KERNEL_TIMER_H__
#define __CO_OS_KERNEL_TIMER_H__

#include <colinux/common/common.h>

typedef struct co_os_timer *co_os_timer_t;
typedef void (*co_os_func_t)(void *);

extern co_rc_t co_os_timer_create(co_os_func_t func, void *data,
				  long msec, co_os_timer_t *timer_out);
extern co_rc_t co_os_timer_activate(co_os_timer_t timer);
extern void co_os_timer_deactivate(co_os_timer_t timer);
extern void co_os_timer_destroy(co_os_timer_t timer);
extern void co_os_msleep(unsigned int msecs);

typedef struct {
	union {
		struct {
			unsigned long low;
			unsigned long high;
		};
		unsigned long long quad;
	};
} co_timestamp_t;

extern void co_os_get_timestamp(co_timestamp_t *dts);
extern void co_os_get_timestamp_freq(co_timestamp_t *dts, co_timestamp_t *freq);
extern unsigned long co_os_get_cpu_khz(void);

#endif

