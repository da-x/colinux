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

#ifdef CO_KERNEL
typedef unsigned long linux_pte_t;
typedef unsigned long linux_pmd_t;
typedef unsigned long linux_pgd_t;
#endif

#include <linux/cooperative.h>

#include "debug.h"

#ifndef NULL
#define NULL (void *)0
#endif

#endif
