/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_REACTOR_H__
#define __COLINUX_OS_USER_REACTOR_H__

#include <colinux/user/reactor.h>

extern co_rc_t co_os_reactor_select(co_reactor_t handle, int miliseconds);

#endif
