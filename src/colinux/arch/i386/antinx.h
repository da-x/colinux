/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_ARCH_I386_ANTINX_H__
#define __COLINUX_ARCH_I386_ANTINX_H__

extern co_rc_t co_arch_anti_nx_init(struct co_monitor *cmon);
extern void co_arch_anti_nx_free(struct co_monitor *cmon);

#endif
