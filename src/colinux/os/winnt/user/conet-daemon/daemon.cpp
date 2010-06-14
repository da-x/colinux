/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>
#include <stdio.h>

extern "C" {
#include <colinux/common/common.h>
#include <colinux/user/cmdline.h>
#include <colinux/user/debug.h>
#include <colinux/user/reactor.h>
#include <colinux/user/monitor.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/user/reactor.h>
#include <colinux/os/current/user/reactor.h>

#include "tap-win32.h"
}

#include "daemon.h"

COLINUX_DEFINE_MODULE("colinux-net-daemon");

user_network_tap_daemon_t::user_network_tap_daemon_t()
{
	tap_handle = NULL;
	tap_name_specified = PFALSE;
}

user_network_tap_daemon_t::~user_network_tap_daemon_t()
{
	if (!tap_handle)
		return;

	co_winnt_reactor_packet_user_destroy(tap_handle);
}

co_module_t user_network_tap_daemon_t::get_base_module()
{
	return CO_MODULE_CONET0;
}

unsigned int user_network_tap_daemon_t::get_unit_count()
{
	return CO_MODULE_MAX_CONET;
}

const char *user_network_tap_daemon_t::get_daemon_name()
{
	return "colinux-net-daemon";
}

const char *user_network_tap_daemon_t::get_daemon_title()
{
	return "Cooperative Linux TAP network daemon";
}

const char *user_network_tap_daemon_t::get_extended_syntax()
{
	return "[-n 'adapter name']";
}

static user_network_tap_daemon_t *tap_daemon = 0;

static co_rc_t tap_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	tap_daemon->received_from_tap(buffer, size);
	return CO_RC(OK);
}

void user_network_tap_daemon_t::prepare_for_loop()
{
	HANDLE win_tap_handle;
	int ret;
	co_rc_t rc;
	char *prefered_name = NULL;

	tap_daemon = this;

	prefered_name = tap_name_specified ? tap_name : NULL;
	if (prefered_name == NULL) {
		log("auto selecting TAP\n");
 	} else {
		log("searching TAP device named \"%s\"\n", prefered_name);
	}

	rc = open_tap_win32(&win_tap_handle, prefered_name);
	if (!CO_OK(rc)) {
		if (CO_RC_GET_CODE(rc) == CO_RC_NOT_FOUND) {
			log("TAP device not found\n");
		} else {
			log("error opening TAP device (%x)\n", rc);
		}
		throw user_daemon_exception_t(CO_RC(ERROR));
	}

	log("enabling TAP...\n");

	ret = tap_win32_set_status(win_tap_handle, TRUE);
	if (!ret) {
		log("error enabling TAP Win32\n");
		throw user_daemon_exception_t(CO_RC(ERROR));
	}

	rc = co_winnt_reactor_packet_user_create(reactor, win_tap_handle, win_tap_handle,
						 tap_receive, &tap_handle);
	if (!CO_OK(rc)) {
		CloseHandle(tap_handle);
		throw user_daemon_exception_t(CO_RC(ERROR));
	}

}

void user_network_tap_daemon_t::received_from_tap(unsigned char *buffer, unsigned long size)
{
	send_to_monitor_raw(CO_DEVICE_NETWORK, buffer, size);
}

void user_network_tap_daemon_t::received_from_monitor(co_message_t *message)
{
	tap_handle->user.send(&tap_handle->user, (unsigned char *)message->data, message->size);
}

void user_network_tap_daemon_t::handle_extended_parameters(co_command_line_params_t cmdline)
{
	co_rc_t rc;

	rc = co_cmdline_params_one_optional_arugment_parameter(
		cmdline, "-n", &tap_name_specified, tap_name, sizeof(tap_name));

	if (!CO_OK(rc)) {
		log("invalid -n paramter\n");
		throw user_daemon_exception_t(CO_RC(ERROR));
	}
}

void user_network_tap_daemon_t::syntax()
{
	user_daemon_t::syntax();
	co_terminal_print("    -n 'adapter name'   The name of the network adapter to attach to\n");
	co_terminal_print("                        Without this option, the daemon tries to\n");
	co_terminal_print("                        guess which interface to use\n");
}


int main(int argc, char *argv[])
{
	user_daemon_t *daemon = 0;

	co_debug_start();
	co_process_high_priority_set();

	try {
		daemon = new user_network_tap_daemon_t;
		daemon->run(argc, argv);
	} catch (user_daemon_exception_t e) {

	}

	if (daemon)
		delete daemon;

	co_debug_end();
	return 0; /* TODO */
}
