/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_MISC_H__
#define __COLINUX_OS_USER_MISC_H__

#include <colinux/common/common.h>

typedef void (*co_terminal_print_hook_func_t)(char *str);

extern co_rc_t co_os_get_physical_ram_size(unsigned long *mem_size);
extern void co_terminal_print(const char *format, ...);
extern void co_set_terminal_print_hook(co_terminal_print_hook_func_t func);

#endif
