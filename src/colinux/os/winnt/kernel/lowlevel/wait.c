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
#include <colinux/os/kernel/wait.h>

struct co_os_wait {
	KEVENT event;
};

co_rc_t co_os_wait_create(co_os_wait_t *wait_out)
{
	co_os_wait_t wait;

	*wait_out = NULL;

	wait = co_os_malloc(sizeof(*wait));

	if (wait == NULL)
		return CO_RC(OUT_OF_MEMORY);

	KeInitializeEvent(&wait->event, SynchronizationEvent, FALSE);

	*wait_out = wait;

	return CO_RC(OK);
}

void co_os_wait_sleep(co_os_wait_t wait)
{
	KeWaitForSingleObject(&wait->event, UserRequest, UserMode, TRUE, NULL);
}

void co_os_wait_wakeup(co_os_wait_t wait)
{
	KeSetEvent(&wait->event, 1, PFALSE);
}

void co_os_wait_destroy(co_os_wait_t wait)
{
	if (wait != NULL)
		co_os_free(wait);
}
