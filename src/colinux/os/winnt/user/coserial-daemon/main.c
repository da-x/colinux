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
#include <conio.h>
#include <ctype.h>

#include <colinux/user/debug.h>
#include <colinux/user/reactor.h>
#include <colinux/user/monitor.h>
#include <colinux/user/cmdline.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/current/user/reactor.h>
#include <colinux/os/winnt/user/misc.h>

COLINUX_DEFINE_MODULE("colinux-serial-daemon");

/*******************************************************************************
 * Type Declarations
 */

typedef struct start_parameters {
	bool_t show_help;
	unsigned int index;
	unsigned int instance;
	bool_t filename_specified;
	char filename [CO_SERIAL_DESC_STR_SIZE];
	bool_t mode_specified;
	char mode [CO_SERIAL_MODE_STR_SIZE];
} start_parameters_t;

/*******************************************************************************
 * Globals
 */
static start_parameters_t g_daemon_parameters;
static co_reactor_t g_reactor;
static co_user_monitor_t *g_monitor_handle;
static co_winnt_reactor_packet_user_t g_reactor_handle;


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


/*
 * Read user input and sent it to the daemon.
 */
static void send_userinput(void)
{
	char buf[256];

	fgets( buf, sizeof(buf), stdin );
	size_t len = strlen( buf );
	if ( len ) {
		std_receive(0, (unsigned char *)buf, len);
	}
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
	co_terminal_print("     colinux-serial-daemon -i pid [-u unit] [-f serial device name] [-m mode]\n");
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

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-f",
			&start_parameters->filename_specified,
			start_parameters->filename, sizeof(start_parameters->filename));
	if (!CO_OK(rc))
		return rc;

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-m",
			&start_parameters->mode_specified,
			start_parameters->mode, sizeof(start_parameters->mode));
	if (!CO_OK(rc))
		return rc;

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
	HANDLE out_handle, in_handle;
	int select_time;

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

	if (g_daemon_parameters.filename_specified == PTRUE) {
		char name [strlen(g_daemon_parameters.filename) + 4+1];
		DCB dcb;
		COMMTIMEOUTS commtimeouts = {
			1,	/* ReadIntervalTimeout */
			0,	/* ReadTotalTimeoutMultiplier */
			0,	/* ReadTotalTimeoutConstant */
			0,	/* WriteTotalTimeoutMultiplier */
			0 };	/* WriteTotalTimeoutConstant */

		if (g_daemon_parameters.filename[0] != '\\') {
			/* short windows name */
			if (strncasecmp(g_daemon_parameters.filename, "COM", 3) != 0)
				co_terminal_print("warning: host serial device '%s' is not a COM port\n",
						    g_daemon_parameters.filename);
			snprintf(name, sizeof(name), "\\\\.\\%s", g_daemon_parameters.filename);
		} else {
			/* windows full name device */
			strncpy(name, g_daemon_parameters.filename, sizeof(name));
		}

		co_debug("open device '%s'", name);
		out_handle = \
		in_handle = CreateFile (name,
				GENERIC_READ | GENERIC_WRITE, 0, NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
		if (in_handle == INVALID_HANDLE_VALUE) {
			co_terminal_print_last_error(g_daemon_parameters.filename);
			return CO_RC(ERROR);
		}

		if (g_daemon_parameters.mode_specified == PTRUE) {
			co_debug("set mode: %s", g_daemon_parameters.mode);

			if (!GetCommState(in_handle, &dcb)) {
				co_terminal_print_last_error("GetCommState");
				return CO_RC(ERROR);
			}

			/* Set defaults. user can overwrite ot */
			dcb.fOutxCtsFlow = FALSE; /* Disable Handshake */
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
			dcb.fRtsControl = RTS_CONTROL_ENABLE;

			if (!BuildCommDCB(g_daemon_parameters.mode, &dcb)) {
				/*co_terminal_print_last_error("colinux-serial-daemon: BuildCommDCB");*/
				co_terminal_print("colinux-serial-daemon: error in mode parameter '%s'\n",
							g_daemon_parameters.mode);
				return CO_RC(ERROR);
			}

			if (!SetCommState(in_handle, &dcb)) {
				co_terminal_print_last_error("SetCommState");
				return CO_RC(ERROR);
			}
		} else {
			if (!EscapeCommFunction(in_handle, SETDTR)) {
				co_terminal_print_last_error("Warning EscapeCommFunction DTR");
				/* return CO_RC(ERROR); */
			}

			if (!EscapeCommFunction(in_handle, SETRTS)) {
				co_terminal_print_last_error("Warning EscapeCommFunction RTS");
				/* return CO_RC(ERROR); */
			}
		}

		if (!SetCommTimeouts(in_handle, &commtimeouts)) {
			co_terminal_print_last_error("SetCommTimeouts");
			return CO_RC(ERROR);
		}

		if (!SetupComm(in_handle, 2048, 2048)) {
			co_terminal_print_last_error("SetupComm");
			return CO_RC(ERROR);
		}

		select_time = -1;
	} else {
		co_debug("std input/output");
		out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		in_handle  = GetStdHandle(STD_INPUT_HANDLE);
		select_time = 100;
	}

	co_debug("connected");

	rc = co_winnt_reactor_packet_user_create(g_reactor,
						 out_handle, in_handle,
						 std_receive, &g_reactor_handle);
	if (!CO_OK(rc))
		return rc;

	co_terminal_print("colinux-serial-daemon: running\n");

	while (CO_OK(rc)) {
		rc = co_reactor_select(g_reactor, select_time);
		if (CO_RC_GET_CODE(rc) == CO_RC_BROKEN_PIPE)
			return CO_RC(OK); /* Normal, if linux shut down */

		if (!g_daemon_parameters.filename_specified)
			if ( _kbhit() )
				send_userinput();
	}

	return -1;
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
