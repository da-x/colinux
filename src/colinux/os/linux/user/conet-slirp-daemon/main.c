
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
#include <stdint.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/misc.h>
#include <colinux/user/debug.h>
#include <colinux/user/cmdline.h>
#include <colinux/common/common.h>
#include <colinux/user/slirp/libslirp.h>
#include <colinux/user/slirp/ctl.h>

#include "../daemon.h"
#include "../unix.h"

#define PARM_SLIRP_REDIR_TCP 0
#define PARM_SLIRP_REDIR_UDP 1

COLINUX_DEFINE_MODULE("colinux-slirp-net-daemon");

static int net_unit = 0;
static co_daemon_handle_t daemon_handle;

int slirp_can_output(void)
{
	return 1;
}

void slirp_output(const uint8_t *pkt, int pkt_len)
{
	/* Received packet from Slirp */
	struct {
		co_message_t message;
		co_linux_message_t message_linux;
		char data[pkt_len];
	} message;
	
	message.message.from = CO_MODULE_CONET0 + net_unit;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message) - sizeof(message.message);
	message.message_linux.device = CO_DEVICE_NETWORK;
	message.message_linux.unit = net_unit;
	message.message_linux.size = pkt_len;

	memcpy(message.data, pkt, pkt_len);

	co_os_daemon_message_send(daemon_handle, &message.message);
}

static co_rc_t wait_loop(void)
{
	co_rc_t rc;
	int ret;

	rc = co_os_set_blocking(daemon_handle->sock, PFALSE);
	if (!CO_OK(rc))
		return rc;

	while (1) {
		/* Slirp main loop as copied from QEMU. */
		fd_set rfds, wfds, xfds;
		int nfds;
		struct timeval tv;

		nfds = -1;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&xfds);

		slirp_select_fill(&nfds, &rfds, &wfds, &xfds);

		FD_SET(daemon_handle->sock, &rfds);
		if (daemon_handle->sock > nfds)
			nfds = daemon_handle->sock;

		tv.tv_sec = 0;
		tv.tv_usec = 10000;

		ret = select(nfds + 1, &rfds, &wfds, &xfds, &tv);
		if (ret > 0) {
			slirp_select_poll(&rfds, &wfds, &xfds);

			if (FD_ISSET(daemon_handle->sock, &rfds)) {
				/* Received packet from daemon */
				co_message_t *message = NULL;
				rc = co_os_daemon_get_message_ready(daemon_handle, &message);
				if (!CO_OK(rc))
					return rc;
				if (message) {
					slirp_input(message->data, message->size);
					co_os_free(message);
				}
			}
		} 

		if (ret < 0)
			break;
	}

	return rc;
}

static void syntax(void)
{
	co_terminal_print("Cooperative Linux Slirp Virtual Network Daemon\n");
	co_terminal_print("Dan Aloni, 2004 (c)\n");
	co_terminal_print("\n");
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("  colinux-slirp-net-daemon -c 0 -i index [-h]\n");
	co_terminal_print("\n");
	co_terminal_print("    -h                      Show this help text\n");
	co_terminal_print("    -i index                Network device index number (0 for eth0, 1 for\n");
	co_terminal_print("                            eth1, etc.)\n");
	co_terminal_print("    -c instance             coLinux instance ID to connect to\n");
	co_terminal_print("    -r tcp|udp:hport:cport  port redirection.\n");
}

static co_rc_t 
parse_redir_param (char *p)
{
	int iProto, iHostPort, iClientPort;
	struct in_addr guest_addr;

	if (!inet_aton(CTL_LOCAL, &guest_addr))
	{
		co_terminal_print("conet-slirp-daemon: slirp redir setup guest_addr failed.\n");
		return CO_RC(ERROR);
	}

	do {
		// minimal len is "tcp:x:x"
		if (strlen (p) < 7)
			return CO_RC(ERROR);

		if (strncasecmp(p, "tcp", 3) == 0)
			iProto = PARM_SLIRP_REDIR_TCP;
		else if (strncasecmp(p, "udp", 3) == 0)
			iProto = PARM_SLIRP_REDIR_UDP;
		else
			return CO_RC(ERROR);

		// check first ':'
		p += 3;
		if (*p != ':')
			return CO_RC(ERROR);
		iHostPort = strtol(p+1, &p, 10);

		// check second ':'
		if (*p != ':')
			return CO_RC(ERROR);
		iClientPort = strtol(p+1, &p, 10);

		co_debug("slirp redir %d %d:%d\n", iProto, iHostPort, iClientPort);
		if (slirp_redir(iProto, iHostPort, guest_addr, iClientPort) < 0) {
			co_terminal_print("conet-slirp-daemon: slirp redir %d:%d failed.\n", iHostPort, iClientPort);
		}

		// Next redirection?
	} while (*p++ == '/');

	return CO_RC(OK);
}

co_rc_t daemon_mode(int unit, int instance)
{
	co_rc_t rc = CO_RC(ERROR);

	rc = co_os_daemon_pipe_open(instance, CO_MODULE_CONET0 + unit, &daemon_handle);
	if (!CO_OK(rc)) {
		co_terminal_print("conet-slirp-daemon: error opening a pipe to the daemon\n");
		goto out;
	}

	rc = wait_loop();

	co_os_daemon_pipe_close(daemon_handle);
out:
	return rc;
}

static co_rc_t co_slirp_main(int argc, char *argv[])
{
	co_rc_t rc;
	co_command_line_params_t cmdline;
	int unit = 0;
	bool_t unit_specified = PFALSE;
	int instance = 0;
	bool_t instance_specified = PFALSE;
	char redir_buff [0x100];
	bool_t redir_specified;
	
	co_debug_start();

	rc = co_cmdline_params_alloc(&argv[1], argc-1, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("conet-slirp-daemon: error parsing arguments\n");
		goto out_clean;
	}

	rc = co_cmdline_params_one_arugment_int_parameter(cmdline, "-c", 
							  &instance_specified, &instance);

	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	rc = co_cmdline_params_one_arugment_int_parameter(cmdline, "-i", 
							  &unit_specified, &unit);

	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	net_unit = unit;

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-r", &redir_specified,
						      redir_buff, sizeof(redir_buff));
	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	rc = co_cmdline_params_check_for_no_unparsed_parameters(cmdline, PTRUE);
	if (!CO_OK(rc)) {
		syntax();
		goto out;
	}

	slirp_init();

	if (redir_specified) {
		rc = parse_redir_param(redir_buff);
		if (!CO_OK(rc)) {
			co_terminal_print("conet-slirp-daemon: Error in redirection '%s'\n", redir_buff);
			return rc;
		}
	}

	co_terminal_print("conet-slirp-daemon: slirp initialized\n");

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

	rc = co_slirp_main(argc, argv);
	if (CO_OK(rc))
		return 0;
		
	return -1;
}
