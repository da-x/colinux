/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "linux_inc.h"

#include <colinux/os/alloc.h>
#include <colinux/os/timer.h>

struct co_os_timer {
	struct timer_list timer;
	long msec;
	co_os_func_t func;
	void *data;
};

static void os_timer(unsigned long data)
{
	struct co_os_timer *t = (struct co_os_timer *)data;
	t->func(t->data);
	t->timer.expires = jiffies + (t->msec+HZ-1)/HZ;
	add_timer((struct timer_list *)&t->timer);
}

co_rc_t co_os_timer_create(co_os_func_t func, void *data,
			   long msec, co_os_timer_t *timer_out)
{
	struct co_os_timer *t;

	t = co_os_malloc(sizeof(*t));
	if (!t)
		return CO_RC(OUT_OF_MEMORY);

	init_timer(&t->timer);

	t->timer.data = (unsigned long)t;
	t->timer.expires = jiffies + (msec+HZ-1)/HZ;
	t->timer.function = os_timer;
	t->msec = msec;
	t->func = func;
	t->data = data;

	*timer_out = (co_os_timer_t)t;

	return CO_RC(OK);
}

co_rc_t co_os_timer_activate(co_os_timer_t timer)
{
	add_timer((struct timer_list *)timer);
	return CO_RC(OK);
}

void co_os_timer_deactivate(co_os_timer_t timer)
{
	del_timer((struct timer_list *)timer);
}

void co_os_timer_destroy(co_os_timer_t timer)
{
	co_os_free((struct timer_list *)timer);
}

void co_os_msleep(unsigned int msecs)
{
	msleep(msecs);
}
