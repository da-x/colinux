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
#include <colinux/user/cmdline.h>
#include <colinux/common/common.h>

#include "../daemon.h"
#include "../unix.h"

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

	co_terminal_print("coserial: loop running\n");

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

	co_terminal_print("coserial: loop stopped\n");

	return rc;
}

static void syntax(void)
{
	co_terminal_print("coserial daemon\n");
	co_terminal_print("Syntax:\n");
	co_terminal_print("       colinux-serial-daemon ([-i colinux instance number] [-u unit] -c [cmdline])|(-m xterm)\n");
}

static co_rc_t create_child(const char *command_line, pid_t *out_pid, int *read_fd, int *write_fd)
{
	int to_pipefd[2], from_pipefd[2];
	int sysrc;
	pid_t pid;

	sysrc = pipe(to_pipefd);
	if (sysrc == -1)
		return CO_RC(ERROR);

	sysrc = pipe(from_pipefd);
	if (sysrc == -1) {
		close(to_pipefd[0]);
		close(to_pipefd[1]);
		return CO_RC(ERROR);
	}

	pid = fork();
	if (pid == 0) {
		int i, fd; 

		setpgrp();

		dup2(to_pipefd[0], 3);
		dup2(from_pipefd[1], 4);

		for (i=0; i <= 255; i++)
			if (i != 3  &&  i != 4)
				close(i);

		dup2(3, 0);
		dup2(4, 1);

		fd = open("/dev/null", O_RDWR);
		if (fd != 2) {
			dup2(fd, 2);
			close(fd);
		}

		sysrc = system(command_line);
		if (sysrc == -1)
			exit(-1);

		if (WIFEXITED(sysrc))
			exit(WEXITSTATUS(sysrc));

		exit(-1);
	}

	if (pid == -1) {
		close(to_pipefd[0]);
		close(to_pipefd[1]);
		close(from_pipefd[0]);
		close(from_pipefd[1]);
		return CO_RC(ERROR);
	}

	*read_fd = from_pipefd[0];
	*write_fd = to_pipefd[1];
	*out_pid = pid;

	close(from_pipefd[1]);
	close(to_pipefd[0]);

	return CO_RC(OK);
}

co_rc_t daemon_mode(bool_t sub_command_line_specified, char *sub_command_line, 
		    int unit, int instance)
{
	int read_fd, write_fd;
	co_daemon_handle_t daemon_handle_;
	pid_t pid;
	co_rc_t rc = CO_RC(ERROR);

	if (!sub_command_line_specified) {
		co_terminal_print("coserial: command line arguemnt -c not specified\n");
		goto out;
	}

	rc = create_child(sub_command_line, &pid, &read_fd, &write_fd);
	if (!CO_OK(rc)) {
		co_terminal_print("coserial: error creating child process\n");
		goto out;
	}
		
	rc = co_os_open_daemon_pipe(instance, CO_MODULE_SERIAL0 + unit, &daemon_handle_);
	if (!CO_OK(rc)) {
		co_terminal_print("coserial: error opening a pipe to the daemon\n");
		goto out_close;
	}

	rc = wait_loop(daemon_handle_, unit, read_fd, write_fd);

	co_os_daemon_close(daemon_handle_);

out_close:
	close(read_fd);
	close(write_fd);

	kill(-pid, SIGINT);
	waitpid(pid, NULL, 0);

out:
	return rc;
}

co_rc_t xterm_mode(void)
{
	struct pollfd pollarray[2];
	int pipes[2][2] = {{0,1},{3,4}};
	char buffer[0x1000];
	co_rc_t rc;
	struct termios term;

	rc = poll_init_socket(&pollarray[0], pipes[0][0]);
	if (!CO_OK(rc))
		return rc;

	rc = poll_init_socket(&pollarray[1], pipes[1][0]);
	if (!CO_OK(rc))
		return rc;

	tcgetattr(pipes[0][0], &term);
	cfmakeraw(&term);
	tcsetattr(pipes[0][0], 0, &term);
	 
	while (1) {
		int ret, i;
		
		pollarray[0].revents = 0;
		pollarray[1].revents = 0;

		ret = poll(pollarray, 2, -1);
		if (ret < 0)
			break;
 
		for (i=0; i < 2; i++) {
			if (pollarray[i].revents & POLLIN) {
				int nread;
				nread = read(pipes[i][0], buffer, sizeof(buffer));
				if (nread) {
					co_os_sendn(pipes[!i][1], buffer, nread);
				}
			}
		}
	}

	return CO_RC(OK);
}

static co_rc_t coserial_main(int argc, char *argv[])
{
	co_rc_t rc;
	co_command_line_params_t cmdline;
	int unit = 0;
	bool_t unit_specified = PFALSE;
	int instance = 0;
	bool_t instance_specified = PFALSE;
	char sub_command_line[0x100];
	char mode[0x100];
	bool_t mode_specified = PFALSE;
	bool_t sub_command_line_specified = PFALSE;
	
	rc = co_cmdline_params_alloc(&argv[1], argc-1, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("coserial: error parsing arguments\n");
		return rc;
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

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-c", 
						      &sub_command_line_specified, 
						      sub_command_line, sizeof(sub_command_line));

	if (!CO_OK(rc))
		goto out;

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-m", 
						      &mode_specified, 
						      mode, sizeof(mode));

	if (!CO_OK(rc))
		goto out;


	rc = co_cmdline_params_check_for_no_unparsed_parameters(cmdline, PTRUE);
	if (!CO_OK(rc))
		goto out;

	if (mode_specified) {
		if (strcmp(mode, "xterm") == 0) {
			xterm_mode();
		}
	}
	else {
		rc = daemon_mode(sub_command_line_specified, sub_command_line, unit, instance);
	}

out:
	co_cmdline_params_free(cmdline);

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
