
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
#include <string.h>
#include <stdlib.h>

#include <colinux/user/debug.h>
#include <colinux/user/reactor.h>
#include <colinux/user/monitor.h>
#include <colinux/user/cmdline.h>
#include <colinux/user/slirp/libslirp.h>
#include <colinux/user/slirp/ctl.h>
#include <colinux/user/slirp/co_main.h>
#include <colinux/os/user/misc.h>

/*******************************************************************************
 * Type Declarations
 */

#define PARM_SLIRP_REDIR_TCP 0
#define PARM_SLIRP_REDIR_UDP 1

typedef struct start_parameters {
	bool_t show_help;
	unsigned int index;
	co_id_t instance;
} start_parameters_t;

/*******************************************************************************
 * Globals
 */
static start_parameters_t g_daemon_parameters;
static co_reactor_t g_reactor;
static co_user_monitor_t *g_monitor_handle;

/* from slirp.c */
extern struct in_addr client_addr;

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
			co_slirp_mutex_lock();
			slirp_input(message->data, message->size);
			co_slirp_mutex_unlock();
		}
		position += message_size;
	}

	return CO_RC(OK);
}

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

	message.message.from = CO_MODULE_CONET0 + g_daemon_parameters.index;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message) - sizeof(message.message);
	message.message_linux.device = CO_DEVICE_NETWORK;
	message.message_linux.unit = g_daemon_parameters.index;
	message.message_linux.size = pkt_len;
	memcpy(message.data, pkt, pkt_len);

	g_monitor_handle->reactor_user->send(g_monitor_handle->reactor_user,
					     (unsigned char *)&message, sizeof(message));
}

static co_rc_t wait_loop(void)
{
	int ret, nfds;
	fd_set rfds, wfds, xfds;
	struct timeval tv;
	co_rc_t rc;

	while (1) {
		/* Slirp main loop as copied from QEMU. */
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

	return rc;
}

/********************************************************************************
 * parameters
 */

static void syntax(void)
{
	co_terminal_print("Cooperative Linux Slirp Virtual Network Daemon\n");
	co_terminal_print("Dan Aloni, 2004 (c)\n");
	co_terminal_print("\n");
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("  colinux-slirp-net-daemon -i pid -u unit [-h]\n");
	co_terminal_print("\n");
	co_terminal_print("    -h                      Show this help text\n");
	co_terminal_print("    -i pid                  coLinux instance ID to connect to\n");
	co_terminal_print("    -u unit                 Network device index number (0 for eth0, 1 for\n");
	co_terminal_print("                            eth1, etc.)\n");
	co_terminal_print("    -r tcp|udp:hport:cport[:count]  port redirection.\n");
}

static co_rc_t
parse_redir_param (char *p)
{
	int iProto, iHostPort, iClientPort, iPortCount, i;

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

		// check optional third ':'
		iPortCount = 1;
		if (*p == ':')
			iPortCount = strtol(p+1, &p, 10);

		if (iPortCount <= 0)
			return CO_RC(ERROR);

		for (i = 0; i < iPortCount; i++) {
			co_debug("slirp redir %d %d:%d", iProto, iHostPort+i, iClientPort+i);
			if (slirp_redir(iProto, iHostPort+i, client_addr, iClientPort+i) < 0) {
				co_terminal_print("conet-slirp-daemon: slirp redir %d:%d failed.\n",
						  iHostPort+i, iClientPort+i);
			}
		}

		// Next redirection?
	} while (*p++ == '/');

	return CO_RC(OK);
}

static co_rc_t
co_slirp_parse_args(co_command_line_params_t cmdline, start_parameters_t *parameters)
{
	co_rc_t rc;
	char redir_buff [0x100];
	bool_t instance_specified;
	bool_t unit_specified;
	bool_t redir_specified;

	/* Parse command line */
	rc = co_cmdline_params_one_arugment_int_parameter(cmdline, "-i",
							  &instance_specified, &parameters->instance);
	if (!CO_OK(rc))
		return rc;

	rc = co_cmdline_params_one_arugment_int_parameter(cmdline, "-u",
							  &unit_specified, &parameters->index);
	if (!CO_OK(rc))
		return rc;

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-r", &redir_specified,
						      redir_buff, sizeof(redir_buff));
	if (!CO_OK(rc))
		return rc;

	rc = co_cmdline_params_argumentless_parameter(cmdline, "-h", &parameters->show_help);
	if (!CO_OK(rc))
		return rc;

	if (parameters->show_help)
		return CO_RC(OK);

	rc = co_cmdline_params_check_for_no_unparsed_parameters(cmdline, PTRUE);
	if (!CO_OK(rc)) {
		syntax();
		return rc;
	}

	if (!unit_specified) {
		co_terminal_print("conet-slirp-daemon: device index not specified\n");
		return CO_RC(ERROR);
	}

	if ((parameters->index < 0) ||
	    (parameters->index >= CO_MODULE_MAX_CONET))
	{
		co_terminal_print("conet-slirp-daemon: invalid index: %d\n", parameters->index);
		return CO_RC(ERROR);
	}

	if (!instance_specified) {
		co_terminal_print("conet-slirp-daemon: coLinux instance not specificed\n");
		return CO_RC(ERROR);
	}

	if (redir_specified) {
		rc = parse_redir_param(redir_buff);
		if (!CO_OK(rc)) {
			co_terminal_print("conet-slirp-daemon: Error in redirection '%s'\n", redir_buff);
			return rc;
		}
	}

	return CO_RC(OK);
}

co_rc_t co_slirp_main(int argc, char *argv[])
{
	co_command_line_params_t cmdline;
	co_rc_t rc;
	co_module_t module;

	co_debug_start();
	co_process_high_priority_set();
	slirp_init();

	rc = co_cmdline_params_alloc(argv+1, argc-1, &cmdline);
	if (!CO_OK(rc))
		goto out;

	rc = co_slirp_parse_args(cmdline, &g_daemon_parameters);
	if (!CO_OK(rc))
		goto out_params;

	if (g_daemon_parameters.show_help) {
		syntax();
		goto out;
	}

	co_debug("conet-slirp-daemon: create mutex");
	rc = co_slirp_mutex_init();
	if (!CO_OK(rc))
		goto out_params;

	co_debug("conet-slirp-daemon: create reactor");
	rc = co_reactor_create(&g_reactor);
	if (!CO_OK(rc))
		goto out_mutex;

	co_debug("conet-slirp-daemon: connecting to monitor");

	module = CO_MODULE_CONET0 + g_daemon_parameters.index;
	rc = co_user_monitor_open(g_reactor, monitor_receive,
				  g_daemon_parameters.instance,
				  &module, 1,
				  &g_monitor_handle);
	if (!CO_OK(rc)) {
		co_terminal_print("conet-slirp-daemon: monitor open failed\n");
		goto out_close;
	}

	co_terminal_print("conet-slirp-daemon: running\n");

	wait_loop();

out_close:
	co_reactor_destroy(g_reactor);

out_mutex:
	co_slirp_mutex_destroy();

out_params:
	co_cmdline_params_free(cmdline);

out:
	if (!CO_OK(rc))
		co_terminal_print("conet-slirp-daemon: exitcode %x\n", (int)rc);

	co_debug_end();
	return rc;
}
