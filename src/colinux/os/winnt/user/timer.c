/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>

#include <colinux/os/alloc.h>
#include <colinux/os/timer.h>

struct co_os_timer {
	UINT event_id;
	co_os_func_t func;
	void *data;
	long msec;
};

static co_os_timer_t global_timer = NULL;

VOID CALLBACK co_os_timer_routine(
	HWND hwnd,
	UINT uMsg,
	UINT_PTR idEvent,
	DWORD dwTime
	)
{
	co_os_timer_t timer = global_timer;

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
	global_timer = timer;

	timer->event_id = SetTimer(NULL, 0, timer->msec, &co_os_timer_routine);
	if (timer->event_id == 0)
		return CO_RC(ERROR);

	return CO_RC(OK);
}

void co_os_timer_deactivate(co_os_timer_t timer)
{
	KillTimer(NULL, timer->event_id);

	global_timer = NULL;
}

void co_os_timer_destroy(co_os_timer_t timer)
{
	if (timer != NULL)
		co_os_free(timer);
}

void co_os_get_timestamp(co_timestamp_t *dts)
{
	LARGE_INTEGER PerformanceCounter;

	QueryPerformanceCounter(&PerformanceCounter);

	dts->high = PerformanceCounter.HighPart;
	dts->low = PerformanceCounter.LowPart;
}
