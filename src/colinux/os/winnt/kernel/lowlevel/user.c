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

/*
 * Ugly for now, but handles exceptions.
 */		     
EXCEPTION_DISPOSITION ProbeForReadHandler(EXCEPTION_RECORD *record, 
					   ULONG *EstablisherFrame, 
					   struct _CONTEXT *ContextRecord,
					   ULONG *DispatcherContext)
{
	EstablisherFrame[2] = 1;
	ContextRecord->Esp = &EstablisherFrame[0];
	ContextRecord->Eip = EstablisherFrame[-4];

	return ExceptionContinueExecution;
}

int co_ProbeForRead(IN CONST VOID  *Address, IN SIZE_T  Length, IN ULONG  Alignment);
asm(
".globl _co_ProbeForRead\n"
"_co_ProbeForRead:\n"
"pushl %ebp;"
"movl %esp, %ebp;"
"pushl %ebx;"
"pushl %esi;"
"pushl %edi;"
"pushl $0;"

"pushl $_ProbeForReadHandler;"
"pushl %fs:0;"
"movl %esp,%fs:0;"
"pushl 16(%ebp);"
"pushl 12(%ebp);"
"pushl 8(%ebp);"
"call __imp__ProbeForRead@12;"
"popl %fs:0;"
"popl %ecx;"

"popl %eax;"
"popl %edi;"
"popl %esi;"
"popl %ebx;"
"popl %ebp;"
"ret;"
"");

/*
 * Ugly for now, but handles exceptions.
 */		     
EXCEPTION_DISPOSITION ProbeForWriteHandler(EXCEPTION_RECORD *record, 
					   ULONG *EstablisherFrame, 
					   struct _CONTEXT *ContextRecord,
					   ULONG *DispatcherContext)
{
	EstablisherFrame[2] = 1;
	ContextRecord->Esp = &EstablisherFrame[0];
	ContextRecord->Eip = EstablisherFrame[-4];

	return ExceptionContinueExecution;
}

int co_ProbeForWrite(IN CONST VOID  *Address, IN SIZE_T  Length, IN ULONG  Alignment);
asm(
".globl _co_ProbeForWrite\n"
"_co_ProbeForWrite:\n"
"pushl %ebp;"
"movl %esp, %ebp;"
"pushl %ebx;"
"pushl %esi;"
"pushl %edi;"
"pushl $0;"

"pushl $_ProbeForWriteHandler;"
"pushl %fs:0;"
"movl %esp,%fs:0;"
"pushl 16(%ebp);"
"pushl 12(%ebp);"
"pushl 8(%ebp);"
"call __imp__ProbeForWrite@12;"
"popl %fs:0;"
"popl %ecx;"

"popl %eax;"
"popl %edi;"
"popl %esi;"
"popl %ebx;"
"popl %ebp;"
"ret;"
"");

co_rc_t co_copy_to_user(char *user_address, char *kernel_address, unsigned long size)
{
	unsigned long ret;
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

	ret = co_ProbeForRead(user_address, size, 1);
	if (ret) {
		DbgPrint("r %x, %d, %d\n", user_address, size, ret);
		return CO_RC(ERROR);
	}

	ret = co_ProbeForWrite(user_address, size, 1);
	if (ret) {
		DbgPrint("w %x, %d, %d\n", user_address, size, ret);
		return CO_RC(ERROR);
	}

	DbgPrint("x %x, %x, %d\n", user_address, kernel_address, size);

	/* co_memcpy(user_address, kernel_address, size); */
	
	return CO_RC(OK);
}


co_rc_t co_copy_from_user(char *user_address, char *kernel_address, unsigned long size)
{
	/* Not implemented */
	return CO_RC(ERROR);
}
