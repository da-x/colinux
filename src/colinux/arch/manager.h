/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_ARCH_MANAGER_H__
#define __COLINUX_ARCH_MANAGER_H__

#include <colinux/common/common.h>

typedef struct co_archdep_manager *co_archdep_manager_t;
struct co_manager;

typedef struct co_archdep_monitor *co_archdep_monitor_t;
struct co_monitor;

extern co_rc_t co_manager_arch_init(struct co_manager *manager, co_archdep_manager_t *out_archdep);
extern void co_manager_arch_free(co_archdep_manager_t out_archdep);

#endif
