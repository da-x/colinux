/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 * Alejandro R. Sedeno <asedeno@mit.edu>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <windows.h>
#include <stdio.h>

#include <colinux/common/common.h>
#include <colinux/user/cmdline.h>
#include <colinux/user/debug.h>
#include <colinux/user/reactor.h>
#include <colinux/user/monitor.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/user/reactor.h>
#include <colinux/os/current/user/reactor.h>

#include "tap-win32.h"

COLINUX_DEFINE_MODULE("colinux-net-daemon");

/*******************************************************************************
 * Type Declarations
 */

typedef struct start_parameters {
	bool_t show_help;
	int index;
	co_id_t instance;
	bool_t name_specified;
	char interface_name[0x100];
} start_parameters_t;

/*******************************************************************************
 * Globals
 */

co_reactor_t g_reactor = NULL;
co_user_monitor_t *g_monitor_handle = NULL;
co_winnt_reactor_packet_user_t g_tap_handle = NULL;

start_parameters_t *daemon_parameters;

co_rc_t monitor_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	co_message_t *message;
	unsigned long message_size;
	long size_left = size;
	long position = 0;

	while (size_left > 0) {
		message = (typeof(message))(&buffer[position]);
		message_size = message->size + sizeof(*message);
		size_left -= message_size;
		if (size_left >= 0) {
			
			g_tap_handle->user.send(&g_tap_handle->user, message->data, message->size);
			/* TODO */
		}
		position += message_size;
	}

	return CO_RC(OK);
}

co_rc_t tap_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	struct {
		co_message_t message;
		co_linux_message_t linux;
		char data[size];
	} message;

	message.message.from = CO_MODULE_CONET0 + daemon_parameters->index;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message) - sizeof(message.message);
	message.linux.device = CO_DEVICE_NETWORK;
	message.linux.unit = daemon_parameters->index;
	message.linux.size = size;
	memcpy(message.data, buffer, size);
	
	g_monitor_handle->reactor_user->send(g_monitor_handle->reactor_user,
					     (unsigned char *)&message, sizeof(message));

	return CO_RC(OK);
}

/********************************************************************************
 * parameters
 */

void co_net_syntax()
{
	co_terminal_print("Cooperative Linux Virtual Network Daemon\n");
	co_terminal_print("Dan Aloni, 2004 (c)\n");
	co_terminal_print("\n");
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("  colinux-net-daemon -c [pid] -i index [-h] [-n 'adapter name']\n");
	co_terminal_print("\n");
	co_terminal_print("    -h                      Show this help text\n");
	co_terminal_print("    -n 'adapter name'       The name of the network adapter to attach to\n");
	co_terminal_print("                            Without this option, the daemon tries to\n");
	co_terminal_print("                            guess which interface to use\n");
	co_terminal_print("    -i index                Network device index number (0 for eth0, 1 for\n");
	co_terminal_print("                            eth1, etc.)\n");
	co_terminal_print("    -c instance             coLinux instance ID to connect to\n");
}

static co_rc_t 
handle_paramters(start_parameters_t *start_parameters, int argc, char *argv[])
{
	char **param_scan = argv;
	const char *option;

	/* Default settings */
	start_parameters->index = -1;
	start_parameters->instance = -1;
	start_parameters->show_help = PFALSE;
	start_parameters->name_specified = PFALSE;

	/* Parse command line */
	while (*param_scan) {
		option = "-i";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			sscanf(*param_scan, "%d", &start_parameters->index);
			param_scan++;
			continue;
		}

		option = "-c";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			sscanf(*param_scan, "%d", (int *)&start_parameters->instance);
			param_scan++;
			continue;
		}

		option = "-n";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			snprintf(start_parameters->interface_name, 
				 sizeof(start_parameters->interface_name), 
				 "%s", *param_scan);

			start_parameters->name_specified = PTRUE;
			param_scan++;
			continue;
		}

		option = "-h";
		if (strcmp(*param_scan, option) == 0) {
			start_parameters->show_help = PTRUE;
		}

		param_scan++;
	}

	if (start_parameters->index == -1) {
		co_terminal_print("conet-daemon: device index not specified\n");
		return CO_RC(ERROR);
	}

	if ((start_parameters->index < 0) ||
	    (start_parameters->index >= CO_MODULE_MAX_CONET)) 
	{
		co_terminal_print("conet-daemon: invalid index: %d\n", start_parameters->index);
		return CO_RC(ERROR);
	}

	if (start_parameters->instance == -1) {
		co_terminal_print("conet-daemon: coLinux instance not specificed\n");
		return CO_RC(ERROR);
	}

	return CO_RC(OK);	
}

int main(int argc, char *argv[])
{	
	co_rc_t rc;
	bool_t ret;
	HANDLE tap_handle;
	int exit_code = 0;
	start_parameters_t start_parameters;
	char *prefered_name = NULL;
	co_module_t modules[] = {CO_MODULE_CONET0, };

	co_debug_start();

	rc = handle_paramters(&start_parameters, argc, argv);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	daemon_parameters = &start_parameters;

	prefered_name = daemon_parameters->name_specified ? daemon_parameters->interface_name : NULL;
	if (prefered_name == NULL) {
		co_terminal_print("conet-daemon: auto selecting TAP\n");
 	} else {
		co_terminal_print("conet-daemon: searching TAP device named \"%s\"\n", prefered_name);
	}

	rc = open_tap_win32(&tap_handle, prefered_name);
	if (!CO_OK(rc)) {
		if (CO_RC_GET_CODE(rc) == CO_RC_NOT_FOUND) {
			co_terminal_print("conet-daemon: TAP device not found\n");
		} else {
			co_terminal_print("conet-daemon: error opening TAP device (%x)\n", rc);
		}
		exit_code = -1;
		goto out;
	}

	co_terminal_print("conet-daemon: enabling TAP...\n");

	ret = tap_win32_set_status(tap_handle, TRUE);
	if (!ret) {
		co_terminal_print("conet-daemon: error enabling TAP Win32\n");
		exit_code = -1;
		goto out_close;

	}

	rc = co_reactor_create(&g_reactor);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}
	
	modules[0] += start_parameters.index;

	co_terminal_print("conet-daemon: connecting to monitor\n");

	rc = co_user_monitor_open(g_reactor, monitor_receive,
				  start_parameters.instance, modules, 
				  sizeof(modules)/sizeof(co_module_t),
				  &g_monitor_handle);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	co_terminal_print("conet-daemon: connected\n");

	rc = co_winnt_reactor_packet_user_create(g_reactor, tap_handle, tap_handle,
						 tap_receive, &g_tap_handle);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	while (1) {
		co_rc_t rc;
		rc = co_reactor_select(g_reactor, 10);
		if (!CO_OK(rc))
			break;
	}

out_close:
	CloseHandle(tap_handle);

out:
	co_debug_end();
	return exit_code;
}
