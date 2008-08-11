/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_REVERSEDPFNS_H__
#define __COLINUX_KERNEL_REVERSEDPFNS_H__

#include <colinux/arch/manager.h>

extern co_rc_t co_manager_set_reversed_pfn(co_manager_t *manager, co_pfn_t real_pfn, co_pfn_t pseudo_pfn);
extern void co_manager_free_reversed_pfns(co_manager_t *manager);
extern co_rc_t co_manager_alloc_reversed_pfns(co_manager_t *manager);

#endif
