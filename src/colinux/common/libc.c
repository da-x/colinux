/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <stdlib.h>
#include <strings.h>
#include <memory.h>

#include "libc.h"

void *co_memset(void *s, int c, long n)
{
	return memset(s, c, n);
}

void *co_memcpy(void *dest, const void *src, long n)
{
	return memcpy(dest, src, n);
}

void co_bzero(void *s, long n)
{
	/* bzero is nowhere define in any of ming32's headers, urgh */
	bzero(s, n);
}

