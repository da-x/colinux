/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_MANAGER_PAGES_H__
#define __COLINUX_KERNEL_MANAGER_PAGES_H__

#include "manager.h"

co_rc_t co_manager_get_page(struct co_manager *manager, co_pfn_t *pfn);

#endif
