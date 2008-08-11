/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_ALLOC_H__
#define __CO_OS_ALLOC_H__

extern void *co_os_malloc(unsigned long bytes);
extern void *co_os_realloc(void *ptr, unsigned long size);
extern void co_os_free(void *ptr);

#endif
