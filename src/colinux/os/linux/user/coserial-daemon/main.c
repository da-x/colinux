/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

#include <colinux/user/debug.h>
#include <colinux/user/reactor.h>
#include <colinux/user/monitor.h>
#include <colinux/user/cmdline.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/current/user/reactor.h>

COLINUX_DEFINE_MODULE("colinux-serial-daemon");

/*******************************************************************************
 * Type Declarations
 */

typedef struct start_parameters {
	bool_t show_help;
	unsigned int index;
	unsigned int instance;
} start_parameters_t;

/*******************************************************************************
 * Globals
 */
static start_parameters_t g_daemon_parameters;
static co_reactor_t g_reactor;
static co_user_monitor_t *g_monitor_handle;
static co_linux_reactor_packet_user_t g_reactor_handle;


static co_rc_t monitor_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
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
			g_reactor_handle->user.send(&g_reactor_handle->user, message->data, message->size);
		}
		position += message_size;
	}

	return CO_RC(OK);
}

static co_rc_t std_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	struct {
		co_message_t message;
		co_linux_message_t message_linux;
		char data[size];
	} message;

	/* Received packet from standard input to linux serial */
	message.message.from = CO_MODULE_SERIAL0 + g_daemon_parameters.index;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message) - sizeof(message.message);
	message.message_linux.device = CO_DEVICE_SERIAL;
	message.message_linux.unit = g_daemon_parameters.index;
	message.message_linux.size = size;
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
	co_terminal_print("Dan Aloni, 2005 (c)\n");
	co_terminal_print("\n");
	co_terminal_print("Syntax:\n");
	co_terminal_print("     colinux-serial-daemon -i pid [-u unit]\n");
}

static co_rc_t
handle_parameters(start_parameters_t *start_parameters, int argc, char *argv[])
{
	bool_t instance_specified, unit_specified;
	co_command_line_params_t cmdline;
	co_rc_t rc;

	/* Default settings */
	start_parameters->index = 0;
	start_parameters->instance = 0;

	rc = co_cmdline_params_alloc(&argv[1], argc-1, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("colinux-serial-daemon: error parsing arguments\n");
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

	if (start_parameters->index < 0 || start_parameters->index >= CO_MODULE_MAX_SERIAL) {
		co_terminal_print("colinux-serial-daemon: Invalid index: %d\n", start_parameters->index);
		return CO_RC(INVALID_PARAMETER);
	}

	if (start_parameters->instance == 0) {
		co_terminal_print("colinux-serial-daemon: coLinux instance not specificed\n");
		return CO_RC(INVALID_PARAMETER);
	}

out:
	co_cmdline_params_free(cmdline);

out_clean:
	return rc;
}

static co_rc_t coserial_main(int argc, char *argv[])
{
	co_rc_t rc;
	co_module_t module;
	struct termios term;
	struct termios term_in;
	struct termios term_out;

	rc = handle_parameters(&g_daemon_parameters, argc, argv);
	if (!CO_OK(rc))
		return rc;

	rc = co_reactor_create(&g_reactor);
	if (!CO_OK(rc))
		return rc;

	co_debug("connecting to monitor");

	module = CO_MODULE_SERIAL0 + g_daemon_parameters.index;
	rc = co_user_monitor_open(g_reactor, monitor_receive,
				  g_daemon_parameters.instance,
				  &module, 1,
				  &g_monitor_handle);
	if (!CO_OK(rc))
		return rc;

	co_debug("connected");

	rc = co_linux_reactor_packet_user_create(g_reactor,
						 STDIN_FILENO,
						 std_receive, &g_reactor_handle);
	if (!CO_OK(rc))
		return rc;

	co_terminal_print("colinux-serial-daemon: running\n");

	tcgetattr(STDIN_FILENO, &term);
	term_in = term;
	cfmakeraw(&term);
	tcsetattr(STDIN_FILENO, 0, &term);

	tcgetattr(STDOUT_FILENO, &term);
	term_out = term;
	cfmakeraw(&term);
	tcsetattr(STDOUT_FILENO, 0, &term);

	while (CO_OK(rc)) {
		rc = co_reactor_select(g_reactor, -1);
		if (CO_RC_GET_CODE(rc) == CO_RC_BROKEN_PIPE) {
			rc = CO_RC(OK); /* Normal, if linux shut down */
			break;
		}
	}

	tcsetattr(STDOUT_FILENO, 0, &term_out);
	tcsetattr(STDIN_FILENO, 0, &term_in);

	return rc;
}

int main(int argc, char *argv[])
{
	co_rc_t rc;

	co_debug_start();

	rc = coserial_main(argc, argv);

	if (!CO_OK(rc))
		co_terminal_print("colinux-serial-daemon: exitcode %x\n", (int)rc);

	co_debug_end();

	if (!CO_OK(rc))
		return -1;

	return 0;
}
