/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_OS_KERNEL_MANAGER_H__
#define __COLINUX_OS_KERNEL_MANAGER_H__

#include <colinux/kernel/manager.h>

extern co_rc_t co_os_manager_init(co_manager_t *manager, co_osdep_manager_t *osdep);
extern void co_os_manager_free(co_osdep_manager_t osdep);

#endif
