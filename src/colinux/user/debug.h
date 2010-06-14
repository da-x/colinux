/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_USER_DEBUG_H__
#define __COLINUX_USER_DEBUG_H__

#include <colinux/common/common.h>

extern void co_daemon_trace_point(co_trace_point_info_t *info);
extern void co_debug_start(void);
extern void co_debug_end(void);

#endif
