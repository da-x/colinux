/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_ARCH_I386_MANAGER_H__
#define __COLINUX_ARCH_I386_MANAGER_H__

#include <colinux/common/common.h>
#include <colinux/kernel/manager.h>
#include <colinux/arch/manager.h>
#include <colinux/os/alloc.h>

#include "cpuid.h"

struct co_archdep_manager {
	bool_t has_cpuid;
	unsigned long caps;
};

#endif
