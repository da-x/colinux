/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 * Ballard, Jonathan H. <jhballard@hotmail.com>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

 /*
  * OS independent main window class.
  * Used for building colinux-console-nt.exe
  */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "console.h"
#include "main.h"

extern "C" {
#include <colinux/user/monitor.h>
#include <colinux/user/cmdline.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/alloc.h>
}

console_window_t::console_window_t()
{
	start_parameters.attach_id = CO_INVALID_ID;
	instance_id		   = CO_INVALID_ID;
	widget			   = NULL;
	message_monitor		   = NULL;
	attached		   = PFALSE;
	co_reactor_create(&reactor);
}

console_window_t::~console_window_t()
{
	if (widget) {
		delete widget;
		widget = NULL;
	}
}

const char *console_window_t::get_name()
{
	return "colinux-console";
}

void console_window_t::syntax()
{
	co_terminal_print("\n");
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("    %s [-a id | -p pidfile]\n", get_name());
	co_terminal_print("\n");
	co_terminal_print("      -a id      Specifies the ID of the coLinux instance to connect.\n");
	co_terminal_print("                 The ID is the PID (process ID) of the colinux-daemon\n");
	co_terminal_print("                 processes for that running instance\n");
	co_terminal_print("      -p pidfile Read ID from this file.\n");
	co_terminal_print("\n");
}

co_rc_t console_window_t::parse_args(int argc, char** argv)
{
	bool_t 		pidfile_specified;
	co_pathname_t	pidfile;
	co_command_line_params_t cmdline;
	co_rc_t rc;

	/* Default settings */

	rc = co_cmdline_params_alloc(&argv[1], argc-1, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("console: error parsing arguments\n");
		goto out_clean;
	}

	rc = co_cmdline_params_one_arugment_int_parameter(
			cmdline,
			"-a",
			NULL,
			&start_parameters.attach_id);

	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	rc = co_cmdline_params_one_arugment_parameter(
			cmdline,
			"-p",
			&pidfile_specified,
			pidfile, sizeof(pidfile));

	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	if (pidfile_specified) {
		rc = read_pid_from_file(pidfile, &start_parameters.attach_id);
		if (!CO_OK(rc)) {
			co_terminal_print("error on reading PID from file '%s'\n", pidfile);
			return CO_RC(ERROR);
		}
	}

	rc = co_cmdline_params_check_for_no_unparsed_parameters(cmdline, PTRUE);
	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

out:
	co_cmdline_params_free(cmdline);

out_clean:
	return rc;
}

co_rc_t console_window_t::send_ctrl_alt_del()
{
	if (!attached)
		return CO_RC(ERROR);

	struct {
		co_message_t		 message;
		co_linux_message_t	 linux_msg;
		co_linux_message_power_t data;
	} message;

	message.message.from     = CO_MODULE_DAEMON;
	message.message.to       = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_IMPORTANT;
	message.message.type     = CO_MESSAGE_TYPE_OTHER;
	message.message.size     = sizeof(message.linux_msg) + sizeof(message.data);

	message.linux_msg.device = CO_DEVICE_POWER;
	message.linux_msg.unit   = 0;
	message.linux_msg.size   = sizeof(message.data);

	message.data.type = CO_LINUX_MESSAGE_POWER_ALT_CTRL_DEL;

	co_user_monitor_message_send(message_monitor, &message.message);

	return CO_RC(OK);
}


co_rc_t console_window_t::start()
{
	widget = co_console_widget_create();
	if (!widget)
		return CO_RC(ERROR);

	widget->title("Console - [ONLINE] - [To Exit, Press Window+Alt Keys]");

	log("Cooperative Linux console started\n");

	if (start_parameters.attach_id != CO_INVALID_ID)
		instance_id = start_parameters.attach_id;
	else
		instance_id = find_first_monitor();

	return attach();
}

// Process all messages in buffer
co_rc_t console_window_t::message_receive(co_reactor_user_t user,
	                                  unsigned char*    buffer,
	                                  unsigned long     size)
{
	co_message_t* message;
	unsigned long message_size;
	long          size_left = size;
	long          position  = 0;

	while (size_left > 0) {
		message      = (typeof(message))(&buffer[position]);
		message_size = message->size + sizeof(*message);
		size_left   -= message_size;
		if (size_left >= 0) {
			global_window->event(message);
		}
		position += message_size;
	}

	return CO_RC(OK);
}

co_rc_t console_window_t::attach()
{
	co_module_t                    modules[] = {CO_MODULE_CONSOLE, };
	co_monitor_ioctl_get_console_t get_console;
	co_console_t*                  console = NULL;
	co_rc_t                        rc;

	if (attached)
		return CO_RC(ERROR);

	rc = co_user_monitor_open(reactor,
				  message_receive,
				  instance_id,
				  modules,
				  sizeof(modules) / sizeof(co_module_t),
				  &message_monitor);
	if (!CO_OK(rc)) {
		log("Monitor%d: Error connecting to instance\n", instance_id);
		return rc;
	}

	rc = co_user_monitor_get_console(message_monitor, &get_console);
	if (!CO_OK(rc)) {
		log("Monitor%d: Error getting console\n", instance_id);
		return rc;
	}

	rc = co_console_create(&get_console.config, &console);
	if (!CO_OK(rc))
		return rc;

	widget->set_console(console);

	rc = widget->set_window(this);
	if (!CO_OK(rc))
		return rc;

	widget->redraw();
	widget->title("Console - Cooperative Linux - [To Exit, Press Window+Alt Keys]");

	attached = PTRUE;

	return CO_RC(OK);
}

co_rc_t console_window_t::detach()
{
	if (!attached)
		return CO_RC(ERROR);

	widget->update();

	co_user_monitor_close(message_monitor);
	message_monitor = NULL;

	widget->set_console(0);

	widget->title("Console [detached]");

	attached = PFALSE;

	log("Monitor%d: Detached\n", instance_id);

	return CO_RC(OK);
}

void console_window_t::event(co_message_t *message)
{
	switch (message->from) {

	case CO_MODULE_MONITOR:
	case CO_MODULE_LINUX:{
		co_console_message_t* console_message;

		console_message = (typeof(console_message)) (message->data);
		widget->event(console_message);
		break;
	}

	case CO_MODULE_CONSOLE:{
		if (message_monitor)
			co_user_monitor_message_send(message_monitor, message);
		break;
	}

	default:
		break;
	}
}

// nlucas: this code is identical to console_window_t::handle_scancode
//         in colinux\user\console\console.cpp.
void console_window_t::handle_scancode(co_scan_code_t sc) const
{
	if (!attached)
		return;

	struct {
		co_message_t		message;
		co_linux_message_t	linux;
		co_scan_code_t		code;
	} message;

	message.message.from     = CO_MODULE_CONSOLE;
	message.message.to       = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type     = CO_MESSAGE_TYPE_OTHER;
	message.message.size     = sizeof (message) - sizeof (message.message);
	message.linux.device     = CO_DEVICE_KEYBOARD;
	message.linux.unit       = 0;
	message.linux.size       = sizeof (message.code);
	message.code             = sc;

	co_user_monitor_message_send(message_monitor, &message.message);
}

void console_window_t::log(const char* format, ...) const
{
	char buf[0x100];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof (buf), format, ap);
	va_end(ap);

	co_terminal_print("console: %s", buf);
}

co_rc_t console_window_t::loop(void)
{
	co_rc_t rc;

	rc = widget->loop();
	if (!(CO_OK(rc) && widget))
		return rc;

	rc = co_reactor_select(reactor, 1);
	if (!CO_OK(rc))
		return rc;

	return widget->idle();
}
