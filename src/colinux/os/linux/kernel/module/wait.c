/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "linux_inc.h"

#include <colinux/os/kernel/wait.h>
#include <colinux/os/alloc.h>

struct co_os_wait {
	wait_queue_head_t head;
};

co_rc_t co_os_wait_create(co_os_wait_t *wait_out)
{
	co_os_wait_t wait;

	*wait_out = NULL;

	wait = co_os_malloc(sizeof(*wait));

	if (wait == NULL)
		return CO_RC(OUT_OF_MEMORY);

	init_waitqueue_head(&wait->head);

	*wait_out = wait;

	return CO_RC(OK);
}

void co_os_wait_sleep(co_os_wait_t wait)
{
	DEFINE_WAIT(wait_one);

	prepare_to_wait(&wait->head, &wait_one, TASK_INTERRUPTIBLE);
	schedule();
	finish_wait(&wait->head, &wait_one);
}

void co_os_wait_wakeup(co_os_wait_t wait)
{
	wake_up_interruptible_all(&wait->head);
}

void co_os_wait_destroy(co_os_wait_t wait)
{
	if (wait != NULL)
		co_os_free(wait);
}
