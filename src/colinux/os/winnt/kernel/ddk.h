/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __NESTED_WINNT_DDK_H__
#define __NESTED_WINNT_DDK_H__

#include <windows.h>
#include <ddk/ntapi.h>
#include <ddk/winddk.h>
#include <ddk/ntddk.h>

#undef IoCompleteRequest 

static inline void
IoCompleteRequest(
  IN PIRP  Irp,
  IN CCHAR  PriorityBoost)
{
    asm(
	"movl %0, %%ecx\n"
	"movb %1, %%dl\n"
	"movl __imp_@IofCompleteRequest@8, %%eax\n"
	"call *%%eax\n"
	: 
	: "m" (Irp), "m" (PriorityBoost));
}

#endif
