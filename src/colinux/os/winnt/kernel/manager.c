/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "manager.h"

#include <colinux/common/libc.h>
#include <colinux/os/alloc.h>

co_rc_t co_os_manager_init(co_manager_t *manager, co_osdep_manager_t *osdep)
{
	co_rc_t rc = CO_RC(OK);
	int i;

	*osdep = (typeof(*osdep))(co_os_malloc(sizeof(**osdep)));
	if (*osdep == NULL)
		return CO_RC(ERROR);

	co_global_manager = manager;

	memset(*osdep, 0, sizeof(**osdep));

	co_list_init(&(*osdep)->mdl_list);
	co_list_init(&(*osdep)->mapped_allocated_list);
	co_list_init(&(*osdep)->pages_unused);
	for (i=0; i < PFN_HASH_SIZE; i++)
		co_list_init(&(*osdep)->pages_hash[i]);

	MmResetDriverPaging(&co_global_manager);
	
	rc = co_os_mutex_create(&(*osdep)->mutex);

	return rc;
}

void co_os_manager_free(co_osdep_manager_t osdep)
{
	co_debug("before free: %d mdsl, %d pages\n", osdep->mdls_allocated, osdep->pages_allocated);
	
	co_winnt_free_all_pages(osdep);

	co_os_mutex_destroy(osdep->mutex);
	
	co_debug("after free: %d mdsl, %d pages\n", osdep->mdls_allocated, osdep->pages_allocated);
	
	co_os_free(osdep);
}

co_rc_t co_os_manager_userspace_open(co_manager_open_desc_t opened)
{
	opened->os = co_os_malloc(sizeof(*opened->os));
	if (!opened->os)
		return CO_RC(ERROR);

	memset(opened->os, 0, sizeof(*opened->os));

	return CO_RC(OK);
}

void co_os_manager_userspace_close(co_manager_open_desc_t opened)
{
	KIRQL irql;
	PIRP Irp;

	Irp = opened->os->irp;
	if (Irp) {
		IoAcquireCancelSpinLock(&irql);
		fixme_IoSetCancelRoutine(Irp, NULL);
		opened->os->irp = NULL;
		IoReleaseCancelSpinLock(irql);

		Irp->IoStatus.Status = STATUS_CANCELLED;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	co_os_free(opened->os);
	opened->os = NULL;
}

bool_t co_os_manager_userspace_try_send_direct(
	co_manager_t *manager,
	co_manager_open_desc_t opened,
	co_message_t *message)
{
	PIRP Irp;

	Irp = opened->os->irp;
	if (Irp) {
		KIRQL irql;
		unsigned char *io_buffer;
		unsigned long size = message->size + sizeof(*message);
		unsigned long buffer_size = Irp->IoStatus.Information;

		IoAcquireCancelSpinLock(&irql);
		fixme_IoSetCancelRoutine(Irp, NULL);
		opened->os->irp = NULL;
		IoReleaseCancelSpinLock(irql);

		co_manager_close(manager, opened);

		io_buffer = Irp->AssociatedIrp.SystemBuffer;

		if (size <= buffer_size) {
			co_memcpy(io_buffer, message, size);
			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = size;
			IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
			return PTRUE;
		} else {
			Irp->IoStatus.Status = STATUS_CANCELLED;
			Irp->IoStatus.Information = 0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}
	}

	return PFALSE;
}

co_rc_t co_os_manager_userspace_eof(co_manager_t *manager, co_manager_open_desc_t opened)
{
	PIRP Irp;

	Irp = opened->os->irp;
	if (Irp) {
		KIRQL irql;

		IoAcquireCancelSpinLock(&irql);
		fixme_IoSetCancelRoutine(Irp, NULL);
		opened->os->irp = NULL;
		IoReleaseCancelSpinLock(irql);

		co_manager_close(manager, opened);

		Irp->IoStatus.Status =  STATUS_PIPE_BROKEN;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return PFALSE;
}

co_id_t co_os_current_id(void)
{
	return (co_id_t)(PsGetCurrentProcessId());
}
