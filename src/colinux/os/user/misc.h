/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_MISC_H__
#define __COLINUX_OS_USER_MISC_H__

#include <colinux/common/common.h>

extern unsigned long co_os_get_physical_ram_size(void);
extern void co_terminal_print(const char *format, ...);

#endif
