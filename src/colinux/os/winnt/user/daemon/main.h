/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_USER_WINNT_DAEMON_MAIN_H__
#define __CO_OS_USER_WINNT_DAEMON_MAIN_H__

#include <colinux/common/common.h>

extern void co_winnt_daemon_stop(void);
extern int co_winnt_daemon_main(char *argv[]);
extern bool_t co_running_as_service;

#endif
