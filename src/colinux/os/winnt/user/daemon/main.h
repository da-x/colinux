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

extern co_rc_t co_winnt_daemon_main(co_start_parameters_t *start_parameters);
extern void co_winnt_daemon_stop(void);

extern bool_t 	 co_running_as_service;

#endif
