/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 * Ballard, Jonathan H. <californiakidd@users.sourceforge.net>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "console.h"

extern "C" {
#include <colinux/user/monitor.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/alloc.h>
#include <colinux/os/user/daemon.h>
}

console_window_t::console_window_t()
{
	start_parameters.attach_id = 0;
	attached_id = CO_INVALID_ID;
	state = CO_CONSOLE_STATE_OFFLINE;
	widget = 0;
	daemon_handle = 0;
}

console_window_t::~console_window_t()
{
}

co_rc_t console_window_t::parse_args(int argc, char **argv)
{
	char ** param_scan = argv;

	/* Parse command line */
	while (argc > 0) {
		const char * option;

		option = "-a";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			argc--;

			if (argc <= 0) {
				co_terminal_print
				    ("Parameter of command line option %s not specified\n",
				     option);
				return CO_RC(ERROR);
			}

			start_parameters.attach_id = atoi(*param_scan);
		}

		param_scan++;
		argc--;
	}

	return CO_RC(OK);
}

co_rc_t console_window_t::start()
{
	co_rc_t rc;

	widget = co_console_widget_create();
	if (!widget)
		return CO_RC(ERROR);

	widget->title("Console - [ONLINE] - [To Exit, Press Window+Alt Keys]");

	rc = widget->console_window(this);
	if (!CO_OK(rc))
		return rc;

	log("Coopeartive Linux console started\n");

	if (start_parameters.attach_id != CO_INVALID_ID)
		attached_id = start_parameters.attach_id;

	return online(true);
}

co_rc_t console_window_t::attach()
{
	co_rc_t rc;

	if (state == CO_CONSOLE_STATE_ATTACHED)
		return CO_RC(ERROR);

	rc = co_os_open_daemon_pipe(attached_id, CO_MODULE_CONSOLE, &daemon_handle);
	if (!CO_OK(rc))
		return rc;

	state = CO_CONSOLE_STATE_ONLINE;

	return CO_RC(OK);
}

co_rc_t console_window_t::attached()
{
	state = CO_CONSOLE_STATE_ATTACHED;

	widget->redraw();

	widget->title
	    ("Console - Cooperative Linux - [To Exit, Press Window+Alt Keys]");

	log("Monitor%d: Attached\n", attached_id);

	return CO_RC(OK);
}

co_rc_t console_window_t::attach_anyhow(co_id_t id)
{
	co_rc_t rc;

	if (state == CO_CONSOLE_STATE_ATTACHED) {
		rc = detach();
		if (!CO_OK(rc))
			return rc;
	}

	attached_id = id;
	return attach();
}

co_rc_t console_window_t::detach()
{
	co_console_t * console;

	if (state != CO_CONSOLE_STATE_ATTACHED)
		return CO_RC(ERROR);

	widget->co_console_update();
	console = widget->co_console();

	struct {
		co_message_t message;
		co_daemon_console_message_t console;
		char data[0];
	} * message;

	co_console_pickle(console);

	message =
	    (typeof(message)) (co_os_malloc(sizeof (*message) + console->size));
	if (message) {
		message->message.to = CO_MODULE_DAEMON;
		message->message.from = CO_MODULE_CONSOLE;
		message->message.size =
		    sizeof (message->console) + console->size;
		message->message.type = CO_MESSAGE_TYPE_STRING;
		message->message.priority = CO_PRIORITY_IMPORTANT;
		message->console.type = CO_DAEMON_CONSOLE_MESSAGE_DETACH;
		message->console.size = console->size;
		memcpy(message->data, console, console->size);

		co_os_daemon_send_message(daemon_handle, &message->message);
		co_os_free(message);
	}

	co_console_unpickle(console);
	co_console_destroy(console);

	co_os_daemon_close(daemon_handle);

	daemon_handle = 0;

	widget->co_console(0);

	state = CO_CONSOLE_STATE_DETACHED;

	widget->title("Console [detached]");

	log("Monitor%d: Detached\n", attached_id);

	return CO_RC(OK);
}

co_rc_t console_window_t::online(const bool ON)
{
	if (ON) {
		if (online())
			return CO_RC(OK);

		if (attached_id != CO_INVALID_ID)
			return attach();

		return CO_RC(ERROR);
	}

	switch (state) {

	case CO_CONSOLE_STATE_ATTACHED:
		detach();

	case CO_CONSOLE_STATE_DETACHED:
		state = CO_CONSOLE_STATE_OFFLINE;

	case CO_CONSOLE_STATE_OFFLINE:
		if (widget)
			delete widget;
		widget = 0;
		return CO_RC(OK);

	case CO_CONSOLE_STATE_ONLINE:
		co_console_t * console;
		co_rc_t rc;

		state = CO_CONSOLE_STATE_OFFLINE;

		/* tries to recover from not being fully attached and sends a blank screen  */
		rc = co_console_create(80, 25, 25, &console);
		if (!CO_OK(rc))
			return rc;

		struct {
			co_message_t
			    message;
			co_daemon_console_message_t
			    console;
			char
			    data[0];
		} *
		    message;

		co_console_pickle(console);

		message =
		    (typeof(message)) (co_os_malloc
				       (sizeof (*message) + console->size));
		if (message) {
			message->message.to = CO_MODULE_DAEMON;
			message->message.from = CO_MODULE_CONSOLE;
			message->message.size =
			    sizeof (message->console) + console->size;
			message->message.type = CO_MESSAGE_TYPE_STRING;
			message->message.priority = CO_PRIORITY_IMPORTANT;
			message->console.type =
			    CO_DAEMON_CONSOLE_MESSAGE_DETACH;
			message->console.size = console->size;
			memcpy(message->data, console, console->size);

			co_os_daemon_send_message(daemon_handle,
						  &message->message);
			co_os_free(message);
		}

		co_console_unpickle(console);
		co_console_destroy(console);

		co_os_daemon_close(daemon_handle);

		daemon_handle = 0;

		return CO_RC(OK);
	}

	return CO_RC(ERROR);
}

void
console_window_t::event(co_message_t & message)
{
	switch (message.from) {

	case CO_MODULE_LINUX:{
			co_console_message_t *
			    console_message;

			console_message =
			    (typeof(console_message)) (message.data);
			widget->event(*console_message);
			break;
		}

	case CO_MODULE_DAEMON:{

			struct {
				co_message_t
				    message;
				co_daemon_console_message_t
				    console;
				char
				    data[0];
			} *
			    console_message;

			console_message = (typeof(console_message)) & message;

			if (console_message->console.type ==
			    CO_DAEMON_CONSOLE_MESSAGE_ATTACH) {
				co_console_t *
				    console;

				console = (co_console_t *)
				    co_os_malloc(console_message->console.size);
				if (!console)
					break;

				memcpy(console, console_message->data,
				       console_message->console.size);
				co_console_unpickle(console);

				widget->co_console(console);

				attached();
			}

			break;
		}

	case CO_MODULE_CONSOLE:{
			if (daemon_handle)
				co_os_daemon_send_message(daemon_handle,
							  &message);
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
	if (state != CO_CONSOLE_STATE_ATTACHED)
		return;

	struct {
		co_message_t		message;
		co_linux_message_t	linux;
		co_scan_code_t		code;
	} message;

	message.message.from = CO_MODULE_CONSOLE;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof (message) - sizeof (message.message);
	message.linux.device = CO_DEVICE_KEYBOARD;
	message.linux.unit = 0;
	message.linux.size = sizeof (message.code);
	message.code = sc;

	co_os_daemon_send_message(daemon_handle, &message.message);
}

void console_window_t::log(const char *format, ...) const
{
	char
	    buf[0x100];
	va_list
	    ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof (buf), format, ap);
	va_end(ap);

	co_debug("Console: %s\n", buf);
}

co_rc_t console_window_t::loop(void)
{
	co_rc_t
	    rc;

	rc = widget->loop();
	if (!(CO_OK(rc)&&widget))
		return rc;

	if (daemon_handle) {
		co_message_t *
		    message = NULL;
		rc = co_os_daemon_get_message(daemon_handle, &message, 10);
		if (!CO_OK(rc)) {
			if (CO_RC_GET_CODE(rc) == CO_RC_BROKEN_PIPE) {
				log("Monitor%d: Broken pipe\n", attached_id);
				online(false);
				return CO_RC(OK);
			}
			if (CO_RC_GET_CODE(rc) != CO_RC_TIMEOUT)
				return rc;
		}

		if (message) {
			event(*message);
			co_os_daemon_deallocate_message(message);
		}
	}

	return widget->idle();
}
