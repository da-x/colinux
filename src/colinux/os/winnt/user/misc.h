/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_USER_WINNT_MISC_H__
#define __CO_OS_USER_WINNT_MISC_H__

#include <colinux/common/common.h>

extern bool_t co_winnt_get_last_error(char *error_message, int buf_size);
extern void co_terminal_print_last_error(const char *message);

#endif
