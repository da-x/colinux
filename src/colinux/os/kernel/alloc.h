/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_KERNEL_ALLOC_H__
#define __CO_OS_KERNEL_ALLOC_H__

#include <colinux/os/alloc.h>

extern void *co_os_alloc_pages(unsigned long pages);
extern void co_os_free_pages(void *ptr, unsigned long pages);

#endif
