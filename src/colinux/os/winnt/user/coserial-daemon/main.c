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
#include <colinux/user/debug.h>
#include <colinux/user/reactor.h>
#include <colinux/user/monitor.h>
#include <colinux/os/user/misc.h>
#include <colinux/user/cmdline.h>
#include <colinux/os/current/user/reactor.h>
#include <colinux/common/common.h>

COLINUX_DEFINE_MODULE("colinux-serial-daemon");

/*******************************************************************************
 * Type Declarations
 */

typedef struct start_parameters {
	bool_t show_help;
	int index;
	int instance;
} start_parameters_t;

/*******************************************************************************
 * Globals
 */
start_parameters_t *daemon_parameters;

co_reactor_t g_reactor = NULL;
co_user_monitor_t *g_monitor_handle = NULL;
co_winnt_reactor_packet_user_t g_std_handle = NULL;

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
			g_std_handle->user.send(&g_std_handle->user, message->data, message->size);
		}
		position += message_size;
	}

	return CO_RC(OK);
}

co_rc_t std_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
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

static void syntax(void)
{
	co_terminal_print("Cooperative Linux Virtual Serial Daemon\n");
	co_terminal_print("Dan Aloni, 2004 (c)\n");
	co_terminal_print("\n");
	co_terminal_print("Syntax:\n");
	co_terminal_print("     colinux-serial-daemon [-i colinux instance number] [-u unit]\n");
}

static co_rc_t 
handle_paramters(start_parameters_t *start_parameters, int argc, char *argv[])
{
	bool_t instance_specified, unit_specified;
	co_command_line_params_t cmdline;
	co_rc_t rc;

	/* Default settings */
	start_parameters->index = 0;
	start_parameters->instance = 0;
	start_parameters->show_help = PFALSE;

	rc = co_cmdline_params_alloc(&argv[1], argc-1, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("coserial: error parsing arguments\n");
		goto out_clean;
	}

	rc = co_cmdline_params_one_arugment_int_parameter(cmdline, "-i", 
							  &instance_specified, &start_parameters->instance);

	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	rc = co_cmdline_params_one_arugment_int_parameter(cmdline, "-u", 
							  &unit_specified, &start_parameters->index);

	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	rc = co_cmdline_params_check_for_no_unparsed_parameters(cmdline, PTRUE);
	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	if ((start_parameters->index < 0) || (start_parameters->index >= CO_MODULE_MAX_SERIAL)) 
	{
		co_terminal_print("Invalid index: %d\n", start_parameters->index);
		return CO_RC(ERROR);
	}

	if (start_parameters->instance == -1) {
		co_terminal_print("coLinux instance not specificed\n");
		return CO_RC(ERROR);
	}

out:
	co_cmdline_params_free(cmdline);

out_clean:
	return rc;
}


int main(int argc, char *argv[])
{	
	co_rc_t rc;
	int exit_code = 0;
	start_parameters_t start_parameters;
	co_module_t modules[] = {CO_MODULE_CONET0, };

	co_debug_start();

	rc = handle_paramters(&start_parameters, argc, argv);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	daemon_parameters = &start_parameters;

	rc = co_reactor_create(&g_reactor);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}
	
	modules[0] += start_parameters.index;

	co_terminal_print("connecting to monitor\n");

	rc = co_user_monitor_open(g_reactor, monitor_receive,
				  start_parameters.instance, modules, 
				  sizeof(modules)/sizeof(co_module_t),
				  &g_monitor_handle);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	co_terminal_print("connected\n");

	rc = co_winnt_reactor_packet_user_create(g_reactor, 
						 GetStdHandle(STD_OUTPUT_HANDLE),
						 GetStdHandle(STD_INPUT_HANDLE), 
						 std_receive, &g_std_handle);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	while (1) {	
		rc = co_reactor_select(g_reactor, 10);
		if (!CO_OK(rc))
			break;
	}

out:
	co_debug_end();
	return exit_code;
}
