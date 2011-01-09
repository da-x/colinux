/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_LINUX_COMMON_H__
#define __COLINUX_LINUX_COMMON_H__

#include "common_base.h"

/*
 * CO_HOST_API is defined when we compile code which interacts
 * with the API of the host OS. In the compilation mode, some types
 * are included by both Linux an the host OS (like, PAGE_SIZE), which
 * can create a problem if we include both of these definition in the
 * same header file. CO_HOST_API is here to avoid such collision form
 * happening by defining common types for both compilation modes.
 */

typedef unsigned long linux_pte_t;
typedef unsigned long linux_pmd_t;
typedef unsigned long linux_pgd_t;

#include <linux/cooperative.h>

/* For C++ NULL should be defined as 0, not as (void*)0 */
#undef NULL
#if defined __cplusplus
# define NULL 	0
#else
# define NULL 	(void*)0
#endif

/*
 * Following is taken from Linux's linux-2.6.33.5-source/lib/div64.c
 */
static inline unsigned long co_div64_32(unsigned long long *n, unsigned long base)
{
	unsigned long long rem = *n;
	unsigned long long b = base;
	unsigned long long res, d = 1;
	unsigned long high = rem >> 32;

	/* Reduce the thing a bit first */
	res = 0;
	if (high >= base) {
		high /= base;
		res = (unsigned long long) high << 32;
		rem -= (unsigned long long) (high*base) << 32;
	}

	while ((signed long long)b > 0 && b < rem) {
		b = b+b;
		d = d+d;
	}

	do {
		if (rem >= b) {
			rem -= b;
			res += d;
		}
		b >>= 1;
		d >>= 1;
	} while (d);

	*n = res;
	return rem;
}

#endif
