/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_LINUX_USER_UNIX_H__
#define __COLINUX_LINUX_USER_UNIX_H__

#include <colinux/common/common.h>

co_rc_t co_os_set_blocking(int sock, bool_t enable);
bool_t co_os_get_blocking(int sock);
co_rc_t co_os_sendn(int sock, char *data, unsigned long size);
co_rc_t co_os_recv(int sock, char *data, unsigned long size, unsigned long *current_size);

#endif
