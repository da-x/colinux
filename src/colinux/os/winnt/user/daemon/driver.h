/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_USER_WINNT_DAEMON_DRIVER_H__
#define __CO_OS_USER_WINNT_DAEMON_DRIVER_H__

#include <colinux/common/common.h>

extern co_rc_t co_winnt_install_driver(void);
extern co_rc_t co_winnt_remove_driver(void);
extern co_rc_t co_winnt_daemon_main_with_driver(char *argv[]);
extern void co_winnt_status_driver(void);

#endif
