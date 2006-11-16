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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/poll.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/misc.h>
#include <colinux/common/common.h>
#include <colinux/user/debug.h>
#include <colinux/user/cmdline.h>
#include "../daemon.h"
#include "../unix.h"

#include "tap.h"

COLINUX_DEFINE_MODULE("colinux-net-daemon");

static int conet_unit = 0;

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

co_rc_t poll_init_socket(struct pollfd *pfd, int fd)
{
	pfd->fd = fd;
	pfd->events = POLLIN | POLLHUP | POLLERR; 
	pfd->revents = 0;

	return co_os_set_blocking(fd, PFALSE);
}

co_rc_t daemon_events(co_daemon_handle_t daemon_handle, int tap, int revents)
{
	co_rc_t rc;
	
	if (revents & POLLIN) {
		co_message_t *message = NULL;
		rc = co_os_daemon_get_message_ready(daemon_handle, &message);
		if (!CO_OK(rc))
			return rc;

		if (message) {
			rc = co_os_sendn(tap, message->data, message->size);
			co_os_free(message);

			if (!CO_OK(rc))
				return rc;
		}
	}

	if (revents & (POLLERR | POLLHUP)) {
		co_terminal_print("daemon closed socket\n");
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

co_rc_t tap_events(co_daemon_handle_t daemon_handle, int tap, int revents)
{
	co_rc_t rc = CO_RC(OK);
	
	if (revents & POLLIN) {
		long read_size;

		/* Received packet from TAP */
		struct {
			co_message_t message;
			co_linux_message_t msg_linux;
			char data[0x1000];
		} message;
		
		read_size = read(tap, message.data, sizeof(message.data));
		if (read_size <= 0)
			return CO_RC(ERROR);
		
		message.message.from = CO_MODULE_CONET0+conet_unit;
		message.message.to = CO_MODULE_LINUX;
		message.message.priority = CO_PRIORITY_DISCARDABLE;
		message.message.type = CO_MESSAGE_TYPE_OTHER;
		message.message.size = sizeof(message.msg_linux) + read_size;
		message.msg_linux.device = CO_DEVICE_NETWORK;
		message.msg_linux.unit = conet_unit;
		message.msg_linux.size = read_size;

		rc = co_os_daemon_message_send(daemon_handle, &message.message);
	}

	if (revents & (POLLERR | POLLHUP)) {
		co_terminal_print("tap was closed socket\n");
		return CO_RC(ERROR);
	}

	return rc;
}

co_rc_t wait_loop(co_daemon_handle_t daemon_handle, int tap)
{
	struct pollfd pollarray[2];
	co_rc_t rc;

	rc = poll_init_socket(&pollarray[0], daemon_handle->sock);
	if (!CO_OK(rc))
		return rc;

	rc = poll_init_socket(&pollarray[1], tap);
	if (!CO_OK(rc))
		return rc;

	co_terminal_print("network loop running\n");

	while (1) {
		int ret;
		
		pollarray[0].revents = 0;
		pollarray[1].revents = 0;
		ret = poll(pollarray, 2, -1);
		if (ret < 0)
			break;

		if (pollarray[0].revents) {
			rc = daemon_events(daemon_handle, tap, pollarray[0].revents);
			if (!CO_OK(rc)) {
				ret = -1;
				break;
			}
		}

		if (pollarray[1].revents) {
			rc = tap_events(daemon_handle, tap, pollarray[1].revents);
			if (!CO_OK(rc)) {
				ret = -1;
				break;
			}
		}
	}

	co_terminal_print("network loop stopped\n");

	return rc;
}

void syntax(void)
{
	co_terminal_print("syntax:\n");
	co_terminal_print("  colinux-net-daemon -c 'colinux instance number'  -i 'conet unit' [-h] [-n 'adapter name']\n");
}

int main(int argc, char *argv[])
{	
	co_rc_t rc;
	int exit_code = 0;
	co_daemon_handle_t daemon_handle_;
	int tap_fd;
	char tap_name[0x30];
	int colinux_instance = 0;
	co_command_line_params_t cmdline;
	bool_t instance_specified = PFALSE;
	bool_t unit_specified = PFALSE;
	bool_t name_specified = PFALSE;

	co_debug_start();

	co_terminal_print("Cooperative Linux TAP network daemon\n");
	if (argc < 3) {
		syntax();
		goto out;
	}

	rc = co_cmdline_params_alloc(&argv[1], argc-1, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("colinux-net-daemon: error parsing arguments\n");
		goto out;
	}

	rc = co_cmdline_params_one_arugment_int_parameter(
			cmdline, "-c", &instance_specified, &colinux_instance);
	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	rc = co_cmdline_params_one_arugment_int_parameter(
			cmdline, "-i", &unit_specified, &conet_unit);
	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	snprintf(tap_name, sizeof(tap_name), "conet-host-%d-%d", colinux_instance, conet_unit);

	rc = co_cmdline_params_one_arugment_parameter(
			cmdline, "-n", &name_specified, tap_name, sizeof(tap_name));
	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	rc = co_cmdline_params_check_for_no_unparsed_parameters(cmdline, PTRUE);
	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	co_terminal_print("creating network %s\n", tap_name);

	tap_fd = tap_alloc(tap_name);
	if (tap_fd < 0) {
		co_terminal_print("Error opening TAP\nn");
		exit_code = -1;		
		goto out;
	}

	co_terminal_print("TAP interface %s created\n", tap_name);

	rc = co_os_daemon_pipe_open(colinux_instance, CO_MODULE_CONET0+conet_unit, &daemon_handle_);
	if (!CO_OK(rc)) {
		co_terminal_print("Error opening a pipe to the daemon\n");
		goto out_close;
	}

	wait_loop(daemon_handle_, tap_fd);
	co_os_daemon_pipe_close(daemon_handle_);

out_close:
	close(tap_fd);

out:
	co_debug_end();
	return exit_code;
}
