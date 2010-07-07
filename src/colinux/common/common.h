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

#include <stdint.h>
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

#ifdef WIN64
static inline uint64_t co_div64(uint64_t a, uint32_t b0)
{
        return a/b0;
}
#else /* WIN64 */
/*
 * Following is taken from Linux's ./arch/i386/kernel/smpboot.c
 */
static inline uint64_t co_div64(uint64_t a, uint32_t b0)
{
        uint32_t a1, a2;
        uint64_t res;

        a1 = ((uint32_t*)&a)[0];
        a2 = ((uint32_t*)&a)[1];

        res = a1/b0 +
                (uint64_t)a2 * (uint64_t)(0xffffffff/b0) +
                a2 / b0 +
                (a2 * (0xffffffff % b0)) / b0;

        return res;
}
#endif /* WIN64 */

#endif
