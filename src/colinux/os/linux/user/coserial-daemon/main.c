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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/misc.h>
#include <colinux/user/debug.h>
#include <colinux/user/cmdline.h>
#include <colinux/common/common.h>

#include "../daemon.h"
#include "../unix.h"

COLINUX_DEFINE_MODULE("colinux-serial-daemon");

static co_rc_t poll_init_socket(struct pollfd *pfd, int fd)
{
	pfd->fd = fd;
	pfd->events = POLLIN | POLLHUP | POLLERR; 
	pfd->revents = 0;

	return co_os_set_blocking(fd, PFALSE);
}

static co_rc_t daemon_events(co_daemon_handle_t daemon_handle, int write_fd, int revents)
{
	co_rc_t rc;
	
	if (revents & POLLIN) {
		co_message_t *message = NULL;

		rc = co_os_daemon_get_message_ready(daemon_handle, &message);
		if (!CO_OK(rc))
			return rc;

		if (message) {
			rc = co_os_sendn(write_fd, message->data, message->size);
			co_os_free(message);

			if (!CO_OK(rc))
				return rc;
		}
	}

	if (revents & (POLLERR | POLLHUP)) {
		co_terminal_print("coserial: daemon closed socket\n");
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

static co_rc_t program_events(co_daemon_handle_t daemon_handle, int unit, int read_fd, int revents)
{
	co_rc_t rc = CO_RC(OK);
	
	if (revents & POLLIN) {
		long read_size;
		struct {
			co_message_t message;
			co_linux_message_t msg_linux;
			char data[2000];
		} message;

		read_size = read(read_fd, message.data, sizeof(message.data));
		if (read_size <= 0)
			return CO_RC(ERROR);

		message.message.from = CO_MODULE_SERIAL0 + unit;
		message.message.to = CO_MODULE_LINUX;
		message.message.priority = CO_PRIORITY_DISCARDABLE;
		message.message.type = CO_MESSAGE_TYPE_OTHER;
		message.message.size = sizeof(message.msg_linux) + read_size;
		message.msg_linux.device = CO_DEVICE_SERIAL;
		message.msg_linux.unit = unit;
		message.msg_linux.size = read_size;

		rc = co_os_daemon_send_message(daemon_handle, &message.message);
	}

	if (revents & (POLLERR | POLLHUP)) {
		return CO_RC(ERROR);
	}

	return rc;
}

static co_rc_t wait_loop(co_daemon_handle_t daemon_handle, int unit, int read_fd, int write_fd)
{
	struct pollfd pollarray[2];
	co_rc_t rc;

	rc = poll_init_socket(&pollarray[0], daemon_handle->sock);
	if (!CO_OK(rc))
		return rc;

	rc = poll_init_socket(&pollarray[1], read_fd);
	if (!CO_OK(rc))
		return rc;

	while (1) {
		int ret;
		
		pollarray[0].revents = 0;
		pollarray[1].revents = 0;
		ret = poll(pollarray, 2, -1);
		if (ret < 0)
			break;

		if (pollarray[0].revents) {
			rc = daemon_events(daemon_handle, write_fd, pollarray[0].revents);
			if (!CO_OK(rc)) {
				ret = -1;
				break;
			}
		}

		if (pollarray[1].revents) {
			rc = program_events(daemon_handle, unit, read_fd, pollarray[1].revents);
			if (!CO_OK(rc)) {
				ret = -1;
				break;
			}
		}
	}

	return rc;
}

static void syntax(void)
{
	co_terminal_print("coserial daemon\n");
	co_terminal_print("Syntax:\n");
	co_terminal_print("       colinux-serial-daemon [-i colinux instance number] [-u unit]\n");
}

co_rc_t daemon_mode(int unit, int instance)
{
	co_daemon_handle_t daemon_handle_;
	co_rc_t rc = CO_RC(ERROR);
	struct termios term;

	rc = co_os_open_daemon_pipe(instance, CO_MODULE_SERIAL0 + unit, &daemon_handle_);
	if (!CO_OK(rc)) {
		co_terminal_print("coserial: error opening a pipe to the daemon\n");
		goto out;
	}

	tcgetattr(0, &term);
	cfmakeraw(&term);
	tcsetattr(0, 0, &term);

	tcgetattr(1, &term);
	cfmakeraw(&term);
	tcsetattr(1, 0, &term);

	rc = wait_loop(daemon_handle_, unit, 0, 1);

	co_os_daemon_close(daemon_handle_);
out:
	return rc;
}

static co_rc_t coserial_main(int argc, char *argv[])
{
	co_rc_t rc;
	co_command_line_params_t cmdline;
	int unit = 0;
	bool_t unit_specified = PFALSE;
	int instance = 0;
	bool_t instance_specified = PFALSE;
	
	co_debug_start();

	rc = co_cmdline_params_alloc(&argv[1], argc-1, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("coserial: error parsing arguments\n");
		goto out_clean;
	}

	rc = co_cmdline_params_one_arugment_int_parameter(cmdline, "-i", 
							  &instance_specified, &instance);

	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	rc = co_cmdline_params_one_arugment_int_parameter(cmdline, "-u", 
							  &unit_specified, &unit);

	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	rc = co_cmdline_params_check_for_no_unparsed_parameters(cmdline, PTRUE);
	if (!CO_OK(rc))
		goto out;


	rc = daemon_mode(unit, instance);

out:
	co_cmdline_params_free(cmdline);

out_clean:
	co_debug_end();

	return rc;
}

int main(int argc, char *argv[])
{	
	co_rc_t rc;

	rc = coserial_main(argc, argv);
	if (CO_OK(rc))
		return 0;
		
	return -1;
}
