/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/poll.h>

#include "daemon.h"

extern "C" {
#include "tap.h"
}

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

	co_linux_reactor_packet_user_destroy(tap_handle);
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

int tap_alloc(char *dev)
{
	int fd;
	int ret;

	if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
		return -1;

	ret = tap_set_name(fd, dev);
	if (ret < 0) {
		close(fd);
		return ret;
	}

	return fd;
}

static user_network_tap_daemon_t *tap_daemon = 0;

static co_rc_t tap_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	tap_daemon->received_from_tap(buffer, size);
	return CO_RC(OK);
}

void user_network_tap_daemon_t::prepare_for_loop()
{
	int tap_fd;
	co_rc_t rc;

	tap_daemon = this;

	if (!tap_name_specified) {
		snprintf(tap_name, sizeof(tap_name), "conet-host-%d-%d", (int)param_instance, param_index);
	}

	log("creating network %s\n", tap_name);

	tap_fd = tap_alloc(tap_name);
	if (tap_fd < 0) {
		log("error opening TAP\n");
		throw user_daemon_exception_t(CO_RC(ERROR));
	}

	log("TAP interface %s created\n", tap_name);

	rc = co_linux_reactor_packet_user_create(reactor, tap_fd, tap_receive, &tap_handle);
	if (!CO_OK(rc)) {
		close(tap_fd);
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
	co_terminal_print("    -n name   Name to create for the network device\n");
}


int main(int argc, char *argv[])
{
	user_daemon_t *daemon = 0;

	co_debug_start();

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
