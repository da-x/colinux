/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "../ddk.h"

#include <excpt.h>

#include <colinux/common/libc.h>
#include <colinux/os/alloc.h>
#include <colinux/os/kernel/user.h>

co_rc_t co_copy_to_user(char *user_address, char *kernel_address, unsigned long size)
{
	PMDL user_mdl;

	if (size == 0)
		return CO_RC(OK);

	user_mdl = IoAllocateMdl(user_address, size, FALSE, FALSE, NULL);
	if (user_mdl) {
		void *vptr;
		MmProbeAndLockPages(user_mdl, KernelMode, IoWriteAccess);
		vptr = MmMapLockedPagesSpecifyCache(user_mdl, KernelMode, MmCached, NULL, FALSE, LowPagePriority);
		if (vptr != NULL) {
			co_memcpy(vptr, kernel_address, size);
			MmUnmapLockedPages(vptr, user_mdl);
		}
		MmUnlockPages(user_mdl);
		IoFreeMdl(user_mdl);
		return CO_RC(OK);
	}

	return CO_RC(ERROR);
}


co_rc_t co_copy_from_user(char *user_address, char *kernel_address, unsigned long size)
{
	PMDL user_mdl;

	if (size == 0)
		return CO_RC(OK);

	user_mdl = IoAllocateMdl(user_address, size, FALSE, FALSE, NULL);
	if (user_mdl) {
		void *vptr;
		MmProbeAndLockPages(user_mdl, KernelMode, IoWriteAccess);
		vptr = MmMapLockedPagesSpecifyCache(user_mdl, KernelMode, MmCached, NULL, FALSE, LowPagePriority);
		if (vptr != NULL) {
			co_memcpy(kernel_address, vptr, size);
			MmUnmapLockedPages(vptr, user_mdl);
		}
		MmUnlockPages(user_mdl);
		IoFreeMdl(user_mdl);
		return CO_RC(OK);
	}

	return CO_RC(ERROR);
}
