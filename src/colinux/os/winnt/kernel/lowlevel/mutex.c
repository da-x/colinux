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
#include <colinux/os/kernel/mutex.h>

struct co_os_mutex {
	KMUTEX mutex;
};

co_rc_t co_os_mutex_create(co_os_mutex_t *mutex_out)
{
	co_os_mutex_t mutex;

	*mutex_out = NULL;

	mutex = co_os_malloc(sizeof(*mutex));

	if (mutex == NULL)
		return CO_RC(OUT_OF_MEMORY);

	KeInitializeMutex(&mutex->mutex, 0);

	*mutex_out = mutex;

	return CO_RC(OK);
}

void co_os_mutex_acquire(co_os_mutex_t mutex)
{
	NTSTATUS status;
	status = KeWaitForMutexObject(&mutex->mutex, UserRequest,
				      KernelMode, FALSE, NULL);

}

void co_os_mutex_acquire_critical(co_os_mutex_t mutex)
{
	if (KeGetCurrentIrql() < DISPATCH_LEVEL)
		co_os_mutex_acquire(mutex);
}

void co_os_mutex_release(co_os_mutex_t mutex)
{
	KeReleaseMutex(&mutex->mutex, FALSE);
}

void co_os_mutex_release_critical(co_os_mutex_t mutex)
{
	if (KeGetCurrentIrql() < DISPATCH_LEVEL)
		co_os_mutex_release(mutex);
}

void co_os_mutex_destroy(co_os_mutex_t mutex)
{
	if (mutex != NULL)
		co_os_free(mutex);
}
