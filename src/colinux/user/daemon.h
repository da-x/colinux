/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __COLINUX_USER_DAEMON_H__
#define __COLINUX_USER_DAEMON_H__

#include <colinux/common/config.h>

#include "elf_load.h"
#include "monitor.h"

typedef struct co_daemon_start_parameters {
	co_pathname_t config_path;
	bool_t launch_console;
	bool_t show_help;
} co_start_parameters_t;

typedef struct co_daemon {
	co_start_parameters_t *start_parameters;
	co_config_t config;
	co_elf_data_t elf_data;
	co_user_monitor_t *monitor;
	char *buf;
} co_daemon_t;

extern void co_daemon_syntax(void);
extern co_rc_t co_daemon_parse_args(char **args, co_start_parameters_t *start_parameters);
extern co_rc_t co_daemon_create(co_start_parameters_t *start_parameters, co_daemon_t **co_daemon_out);
extern co_rc_t co_daemon_start_monitor(co_daemon_t *daemon);
extern co_rc_t co_daemon_run(co_daemon_t *daemon);
extern void co_daemon_end_monitor(co_daemon_t *daemon);
extern void co_daemon_destroy(co_daemon_t *daemon);

#endif
