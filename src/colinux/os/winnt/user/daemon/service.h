/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_USER_WINNT_DAEMON_SERVICE_H__
#define __CO_OS_USER_WINNT_DAEMON_SERVICE_H__

#include "cmdline.h"

extern co_rc_t co_winnt_daemon_install_as_service(const char *service_name, 
						  co_start_parameters_t *start_parameters);
extern int co_winnt_daemon_remove_service(const char *service_name);
extern bool_t co_winnt_daemon_initialize_service(co_start_parameters_t *start_parameters, 
						 const char *service_name);

extern void co_ntevent_print(const char *format, ...);

extern co_rc_t co_winnt_install_driver(void);
extern co_rc_t co_winnt_remove_driver(void);

#endif
