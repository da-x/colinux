/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_LINUX_LIBC_H__
#define __COLINUX_LINUX_LIBC_H__

extern void *co_memset(void *s, int c, long n);
extern void *co_memcpy(void *dest, const void *src, long n);
extern void co_bzero(void *s, long n);

#endif
