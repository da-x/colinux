/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "../ddk.h"

#include <colinux/os/alloc.h>
#include <colinux/os/timer.h>

struct co_os_timer {
	KDPC dpc;
	KTIMER ktimer;
	co_os_func_t func;
	void *data;
	long msec;
};

VOID
DDKAPI
co_os_timer_routine(
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	)
{
	co_os_timer_t timer = (co_os_timer_t)DeferredContext;

	timer->func(timer->data);
}

co_rc_t co_os_timer_create(co_os_func_t func, void *data,
			   long msec, co_os_timer_t *timer_out)
{
	co_os_timer_t timer;

	*timer_out = NULL;

	timer = co_os_malloc(sizeof(*timer));

	if (timer == NULL)
		return CO_RC(OUT_OF_MEMORY);

	timer->func = func;
	timer->data = data;
	timer->msec = msec;

	*timer_out = timer;

	return CO_RC(OK);
}

co_rc_t co_os_timer_activate(co_os_timer_t timer)
{
	LARGE_INTEGER li;

	li.QuadPart = 0;

	KeInitializeDpc(&timer->dpc, &co_os_timer_routine, (PVOID)timer);
	KeInitializeTimerEx(&timer->ktimer, SynchronizationTimer);
	KeSetTimerEx(&timer->ktimer, li, timer->msec, &timer->dpc);

	return CO_RC(OK);
}

void co_os_timer_deactivate(co_os_timer_t timer)
{
	KeCancelTimer(&timer->ktimer);
}

void co_os_timer_destroy(co_os_timer_t timer)
{
	if (timer != NULL)
		co_os_free(timer);
}

void co_os_msleep(unsigned int msecs)
{
	LARGE_INTEGER DueTime;

	DueTime.QuadPart = (long long)msecs * 10000 * (-1);
	KeDelayExecutionThread(KernelMode, FALSE, &DueTime);
}
