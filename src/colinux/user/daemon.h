/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __COLINUX_USER_DAEMON_H__
#define __COLINUX_USER_DAEMON_H__

#include <colinux/common/config.h>
#include <colinux/common/messages.h>
#include <colinux/common/console.h>

#include "elf_load.h"
#include "monitor.h"
#include "console/daemon.h"

typedef struct co_daemon_start_parameters {
	co_pathname_t config_path;
	bool_t launch_console;
	bool_t show_help;
	char console[0x20];
} co_start_parameters_t;

typedef struct co_daemon {
	co_start_parameters_t *start_parameters;
	co_config_t config;
	co_elf_data_t elf_data;
	co_user_monitor_t *monitor;
	co_console_t *console;
	struct co_connected_module *console_module;
	co_message_switch_t message_switch;
	co_list_t connected_modules;
	co_queue_t up_queue;
	bool_t running;
	bool_t idle;
	char *buf;
	double last_htime;
	double reminder_htime;
	bool_t send_ctrl_alt_del;
} co_daemon_t;

typedef enum {
	CO_CONNECTED_MODULE_STATE_NEW,
	CO_CONNECTED_MODULE_STATE_IDENTIFIED,
} co_connected_module_state_t;

#define CO_CONNECTED_MODULE_NAME_SIZE 0x30

typedef struct co_connected_module {
	co_module_t id;
	co_list_t node;
	co_daemon_t *daemon;
	co_connected_module_state_t state;
	struct co_os_pipe_connection *connection;
	char name[CO_CONNECTED_MODULE_NAME_SIZE];
} co_connected_module_t;

extern void co_daemon_syntax(void);
extern void co_daemon_print_header(void);
extern co_rc_t co_daemon_parse_args(char **args, co_start_parameters_t *start_parameters);
extern co_rc_t co_daemon_create(co_start_parameters_t *start_parameters, co_daemon_t **co_daemon_out);
extern co_rc_t co_daemon_start_monitor(co_daemon_t *daemon);
extern co_rc_t co_daemon_run(co_daemon_t *daemon);
extern void co_daemon_end_monitor(co_daemon_t *daemon);
extern void co_daemon_destroy(co_daemon_t *daemon);
extern void co_daemon_send_ctrl_alt_del(co_daemon_t *daemon);

#endif
