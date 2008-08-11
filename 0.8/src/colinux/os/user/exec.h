/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_EXEC_H__
#define __COLINUX_OS_USER_EXEC_H__

#include <colinux/common/common.h>

extern co_rc_t co_launch_process(int *pid, char *command_line, ...);
extern co_rc_t co_kill_process(int pid);

#endif
