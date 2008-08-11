/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_WINNT_KERNEL_EXCEPTIONS_H__
#define __CO_WINNT_KERNEL_EXCEPTIONS_H__

#include "ddk.h"

#include <excpt.h>

#define CO_EXCEPTION_WRAPPER(name, frame_size, pushes)   		\
EXCEPTION_DISPOSITION name##Handler(EXCEPTION_RECORD *record,		\
				    ULONG *EstablisherFrame,		\
				    struct _CONTEXT *ContextRecord,	\
				    ULONG *DispatcherContext)		\
{									\
	(*((unsigned long *)(EstablisherFrame[7]))) = 1;		\
	ContextRecord->Esp = (unsigned long)&EstablisherFrame[0];	\
	ContextRecord->Eip = (unsigned long)EstablisherFrame[-1-(frame_size/4)]; \
									\
	return ExceptionContinueExecution;				\
}									\
									\
asm(									\
".globl _co_" #name "\n"						\
"_co_" #name ":\n"							\
"pushl %ebp;"								\
"movl %esp, %ebp;"							\
"pushl %ebx;"								\
"pushl %esi;"								\
"pushl %edi;"								\
									\
"pushl $_" #name "Handler;"						\
"pushl %fs:0;"								\
"movl %esp,%fs:0;"							\
pushes									\
"call __imp__" #name "@"#frame_size";"					\
"popl %fs:0;"								\
"popl %ecx;"								\
									\
"popl %edi;"								\
"popl %esi;"								\
"popl %ebx;"								\
"popl %ebp;"								\
"ret;"									\
"");									\


#endif
