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
#include <colinux/os/user/misc.h>
#include <colinux/user/slirp/libslirp.h>
#include <colinux/user/slirp/ctl.h>

#include "../daemon.h"

#define PARM_SLIRP_REDIR_TCP 0
#define PARM_SLIRP_REDIR_UDP 1

COLINUX_DEFINE_MODULE("colinux-slirp-net-daemon");

/*******************************************************************************
 * Type Declarations
 */

typedef struct co_win32_overlapped {
	HANDLE handle;
	HANDLE read_event;
	HANDLE write_event;
	OVERLAPPED read_overlapped;
	OVERLAPPED write_overlapped;
	char buffer[0x10000];
	unsigned long size;
} co_win32_overlapped_t;

typedef struct start_parameters {
	bool_t show_help;
	int index;
	co_id_t instance;
} start_parameters_t;

/*******************************************************************************
 * Globals
 */
start_parameters_t *daemon_parameters;
co_win32_overlapped_t daemon_overlapped;

HANDLE slirp_mutex;

/* from slirp.c */
extern struct in_addr client_addr;

#ifdef profile_me
#undef profile_me
void profile_me(char *location)
{      
	LARGE_INTEGER Frequency;
	LARGE_INTEGER Counter;
	double current;
	static double last;

	QueryPerformanceCounter(&Counter);
	QueryPerformanceFrequency(&Frequency);

	current = ((double)Counter.QuadPart) / ((double)Frequency.QuadPart);

        printf("%s: %5.8f\n", location, current - last);

	last = current;
}
#else
#define profile_me(param) do {} while(0);
#endif

co_rc_t co_win32_overlapped_write_async(co_win32_overlapped_t *overlapped,
					void *buffer, unsigned long size)
{
	unsigned long write_size;
	BOOL result;
	DWORD error;

	result = WriteFile(overlapped->handle, buffer, size,
			   &write_size, &overlapped->write_overlapped);
	
	if (!result) { 
		switch (error = GetLastError())
		{ 
		case ERROR_IO_PENDING: 
			WaitForSingleObject(overlapped->write_event, INFINITE);
			break;
		default:
			return CO_RC(ERROR);
		} 
	}

	return CO_RC(OK);
}

co_rc_t co_win32_overlapped_read_received(co_win32_overlapped_t *overlapped)
{
	if (overlapped == &daemon_overlapped) {
		/* Received packet from daemon */
		co_message_t *message;
		char * buffer = overlapped->buffer;
		long size_left = overlapped->size;
		unsigned long message_size;

		do {
			message = (co_message_t *)buffer;
			message_size = message->size + sizeof (co_message_t);
			buffer += message_size;
			size_left -= message_size;

			/* Check buffer overrun */
			if (size_left < 0) {
				co_debug("Error: Message incomplete (%ld)\n", size_left);
				return CO_RC(ERROR);
			}

			profile_me("to slirp");

			WaitForSingleObject(slirp_mutex, INFINITE);
			slirp_input(message->data, message->size);
			ReleaseMutex(slirp_mutex);

		} while (size_left > 0);
	}

	return CO_RC(OK);
}

co_rc_t co_win32_overlapped_read_async(co_win32_overlapped_t *overlapped)
{
	BOOL result;
	DWORD error;

	while (TRUE) {
		result = ReadFile(overlapped->handle,
				  &overlapped->buffer,
				  sizeof(overlapped->buffer),
				  &overlapped->size,
				  &overlapped->read_overlapped);

		if (!result) { 
			error = GetLastError();
			switch (error)
			{ 
			case ERROR_IO_PENDING: 
				return CO_RC(OK);
			default:
				co_debug("Error: %x\n", error);
				return CO_RC(ERROR);
			}
		} else {
			co_win32_overlapped_read_received(overlapped);
		}
	}

	return CO_RC(OK);
}

co_rc_t co_win32_overlapped_read_completed(co_win32_overlapped_t *overlapped)
{
	BOOL result;

	result = GetOverlappedResult(
		overlapped->handle,
		&overlapped->read_overlapped,
		&overlapped->size,
		FALSE);

	if (result) {
		co_win32_overlapped_read_received(overlapped);
		co_win32_overlapped_read_async(overlapped);
	} else {
		if (GetLastError() == ERROR_BROKEN_PIPE) {
			co_debug("Pipe broken, exiting\n");
			return CO_RC(ERROR);
		}

		co_debug("GetOverlappedResult error %d\n", GetLastError());
	}
	 
	return CO_RC(OK);
}

void co_win32_overlapped_init(co_win32_overlapped_t *overlapped, HANDLE handle)
{
	overlapped->handle = handle;
	overlapped->read_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	overlapped->write_event = CreateEvent(NULL, FALSE, FALSE, NULL);

	overlapped->read_overlapped.Offset = 0; 
	overlapped->read_overlapped.OffsetHigh = 0; 
	overlapped->read_overlapped.hEvent = overlapped->read_event; 

	overlapped->write_overlapped.Offset = 0; 
	overlapped->write_overlapped.OffsetHigh = 0; 
	overlapped->write_overlapped.hEvent = overlapped->write_event; 
}

int wait_loop(HANDLE daemon_handle)
{
	HANDLE wait_list[1];
	ULONG status;
	co_rc_t rc;
	int ret;
	fd_set rfds, wfds, xfds;
	int nfds;
	struct timeval tv;
	
	co_win32_overlapped_init(&daemon_overlapped, daemon_handle);
	wait_list[0] = daemon_overlapped.read_event;
	co_win32_overlapped_read_async(&daemon_overlapped); 

	while (1) {
		status = WaitForMultipleObjects(1, wait_list, FALSE, 1); 
			
		switch (status) {
		case WAIT_OBJECT_0: { /* daemon */
			profile_me("from daemon");
			rc = co_win32_overlapped_read_completed(&daemon_overlapped);
			if (!CO_OK(rc))
				return 0;
			break;
		}
		default:
			break;
		}

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
	
	profile_me("to linux");
	
	co_win32_overlapped_write_async(&daemon_overlapped, &message, sizeof(message));
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
		if (slirp_redir(iProto, iHostPort, client_addr, iClientPort) < 0) {
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

static void terminal_print_hook_func(char *str)
{
	struct {
		co_message_t message;
		char data[strlen(str)+1];
	} message;
	
	message.message.from = CO_MODULE_CONET0 + daemon_parameters->index;
	message.message.to = CO_MODULE_CONSOLE;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_STRING;
	message.message.size = strlen(str)+1;
	memcpy(message.data, str, strlen(str)+1);

	if (daemon_overlapped.handle != NULL) {
		co_win32_overlapped_write_async(&daemon_overlapped, &message, sizeof(message));
	}
}

int main(int argc, char *argv[])
{	
	co_rc_t rc;
	HANDLE daemon_handle = 0;
	int exit_code = 0;
	co_daemon_handle_t daemon_handle_;
	start_parameters_t start_parameters;

	co_debug_start();
	slirp_init();

	rc = handle_paramters(&start_parameters, argc, argv);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	co_terminal_print("Slirp initialized\n");

	daemon_parameters = &start_parameters;
	rc = co_os_daemon_pipe_open(daemon_parameters->instance, 
				    CO_MODULE_CONET0 + daemon_parameters->index, &daemon_handle_);
	if (!CO_OK(rc)) {
		co_terminal_print("Error opening a pipe to the daemon\n");
		goto out;
	}

	slirp_mutex = CreateMutex(NULL, FALSE, NULL);
	if (slirp_mutex == NULL) 
		goto out_close;

	co_set_terminal_print_hook(terminal_print_hook_func);

	co_terminal_print("Slirp loop running\n");

	daemon_handle = daemon_handle_->handle;
	exit_code = wait_loop(daemon_handle);

	CloseHandle(slirp_mutex);

out_close:
	co_os_daemon_pipe_close(daemon_handle_);

out:
	co_debug_end();
	return exit_code;
}
