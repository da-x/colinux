/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 * Alejandro R. Sedeno <asedeno@mit.edu>, 2004 (c)
 * Fabrice Bellard, 2003-2004 Copyright (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include <colinux/common/common.h>
#include <colinux/user/debug.h>
#include <colinux/user/reactor.h>
#include <colinux/user/monitor.h>
#include <colinux/user/slirp/libslirp.h>
#include <colinux/user/slirp/ctl.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/current/user/reactor.h>

#define PARM_SLIRP_REDIR_TCP 0
#define PARM_SLIRP_REDIR_UDP 1

COLINUX_DEFINE_MODULE("colinux-slirp-net-daemon");

/*******************************************************************************
 * Type Declarations
 */

typedef struct start_parameters {
	bool_t show_help;
	int index;
	co_id_t instance;
} start_parameters_t;

/*******************************************************************************
 * Globals
 */
start_parameters_t *daemon_parameters;
co_reactor_t g_reactor = NULL;
co_user_monitor_t *g_monitor_handle = NULL;

HANDLE slirp_mutex;

co_rc_t monitor_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
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
			WaitForSingleObject(slirp_mutex, INFINITE);
			slirp_input(message->data, message->size);
			ReleaseMutex(slirp_mutex);
		}
		position += message_size;
	}

	return CO_RC(OK);
}

void slirp_output(const uint8_t *pkt, int pkt_len)
{
	/* Received packet from Slirp */
	struct {
		co_message_t message;
		co_linux_message_t linux;
		char data[pkt_len];
	} message;
	
	message.message.from = CO_MODULE_CONET0 + daemon_parameters->index;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message) - sizeof(message.message);
	message.linux.device = CO_DEVICE_NETWORK;
	message.linux.unit = daemon_parameters->index;
	message.linux.size = pkt_len;
	memcpy(message.data, pkt, pkt_len);
	

	g_monitor_handle->reactor_user->send(g_monitor_handle->reactor_user,
					     (unsigned char *)&message, sizeof(message));
}

int wait_loop()
{
	int ret, nfds;
	fd_set rfds, wfds, xfds;
	struct timeval tv;
	co_rc_t rc;

	while (1) {
		rc = co_reactor_select(g_reactor, 1);
		if (!CO_OK(rc))
			break;

		nfds = -1;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&xfds);

		slirp_select_fill(&nfds, &rfds, &wfds, &xfds);
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		ret = select(nfds + 1, &rfds, &wfds, &xfds, &tv);
		if (ret >= 0) {
			slirp_select_poll(&rfds, &wfds, &xfds);
		}
	}

	return 0;
}

int slirp_can_output(void)
{
	return 1;
}

/********************************************************************************
 * parameters
 */

void co_net_syntax()
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
parse_redir_param (const char *p)
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

		iProto = (memicmp(p, "udp", 3) == 0)
			? PARM_SLIRP_REDIR_UDP : PARM_SLIRP_REDIR_TCP;

		// search the first ':'
		p = strchr (p, ':');
		if (!p)
			return CO_RC(ERROR);
		iHostPort = atol(p+1);

		// search the second ':'
		p = strchr (p+1, ':');
		if (!p)
			return CO_RC(ERROR);
		iClientPort = atol(p+1);

		co_debug("slirp redir %d %d:%d\n", iProto, iHostPort, iClientPort);
		if (slirp_redir(iProto, iHostPort, guest_addr, iClientPort) < 0) {
			co_terminal_print("conet-slirp-daemon: slirp redir %d failed.\n", iClientPort);
		}

		// Next redirection?
		p = strchr (p, '/');
	} while (p++);

	return CO_RC(OK);
}

static co_rc_t 
handle_paramters(start_parameters_t *start_parameters, int argc, char *argv[])
{
	char **param_scan = argv;
	const char *option;
	co_rc_t rc;

	/* Default settings */
	start_parameters->index = -1;
	start_parameters->instance = -1;
	start_parameters->show_help = PFALSE;

	/* Parse command line */
	while (*param_scan) {
		option = "-i";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-slirp-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			start_parameters->index = atoi(*param_scan);
			param_scan++;
			continue;
		}

		option = "-c";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-slirp-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			start_parameters->instance = (co_id_t) atoi(*param_scan);
			param_scan++;
			continue;
		}

		option = "-h";
		if (strcmp(*param_scan, option) == 0) {
			start_parameters->show_help = PTRUE;
		}

		option = "-r";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-slirp-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			rc = parse_redir_param(*param_scan);
			if (!CO_OK(rc)) {
				co_terminal_print("conet-slirp-daemon: parameter of command line option %s not specified\n", option);
				return rc;
			}
			param_scan++;
			continue;
		}
		param_scan++;
	}

	if (start_parameters->index == -1) {
		co_terminal_print("conet-slirp-daemon: device index not specified\n");
		return CO_RC(ERROR);
	}

	if ((start_parameters->index < 0) ||
	    (start_parameters->index >= CO_MODULE_MAX_CONET)) 
	{
		co_terminal_print("conet-slirp-daemon: invalid index: %d\n", start_parameters->index);
		return CO_RC(ERROR);
	}

	if (start_parameters->instance == -1) {
		co_terminal_print("conet-slirp-daemon: coLinux instance not specificed\n");
		return CO_RC(ERROR);
	}

	return CO_RC(OK);	
}

int main(int argc, char *argv[])
{	
	co_rc_t rc;
	int exit_code = 0;
	start_parameters_t start_parameters;
	co_module_t modules[] = {CO_MODULE_CONET0, };
	WSADATA wsad;

	co_debug_start();
	slirp_init();

	rc = handle_paramters(&start_parameters, argc, argv);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	WSAStartup(MAKEWORD(2, 0), &wsad);

	co_terminal_print("conet-slirp-daemon: slirp initialized\n");

	daemon_parameters = &start_parameters;
	slirp_mutex = CreateMutex(NULL, FALSE, NULL);
	if (slirp_mutex == NULL) 
		goto out_close;

	rc = co_reactor_create(&g_reactor);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	co_terminal_print("conet-slirp-daemon: connecting to monitor\n");

	modules[0] += start_parameters.index;
	rc = co_user_monitor_open(g_reactor, monitor_receive,
				  start_parameters.instance, modules, 
				  sizeof(modules)/sizeof(co_module_t),
				  &g_monitor_handle);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	co_terminal_print("conet-slirp-daemon: loop running\n");

	wait_loop();
	
	CloseHandle(slirp_mutex);

out_close:

out:
	co_debug_end();
	return exit_code;
}
