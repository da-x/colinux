/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 * Alejandro R. Sedeno <asedeno@mit.edu>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <windows.h>
#include <stdio.h>

#include <colinux/common/common.h>
#include <colinux/user/debug.h>
#include <colinux/os/user/misc.h>

#include "tap-win32.h"
#include "../daemon.h"

COLINUX_DEFINE_MODULE("colinux-net-daemon");

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
	bool_t name_specified;
	char interface_name[0x100];
} start_parameters_t;

/*******************************************************************************
 * Globals
 */
start_parameters_t *daemon_parameters;

co_win32_overlapped_t tap_overlapped, daemon_overlapped;

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

			profile_me("to tap");

			co_win32_overlapped_write_async(&tap_overlapped, message->data, message->size);

		} while (size_left > 0);

	} else {
		/* Received packet from TAP */
		struct {
			co_message_t message;
			co_linux_message_t linux;
			char data[overlapped->size];
		} message;
		
		message.message.from = CO_MODULE_CONET0 + daemon_parameters->index;
		message.message.to = CO_MODULE_LINUX;
		message.message.priority = CO_PRIORITY_DISCARDABLE;
		message.message.type = CO_MESSAGE_TYPE_OTHER;
		message.message.size = sizeof(message) - sizeof(message.message);
		message.linux.device = CO_DEVICE_NETWORK;
		message.linux.unit = daemon_parameters->index;
		message.linux.size = overlapped->size;
		memcpy(message.data, overlapped->buffer, overlapped->size);

		profile_me("to linux");

		co_win32_overlapped_write_async(&daemon_overlapped, &message, sizeof(message));
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

int wait_loop(HANDLE daemon_handle, HANDLE tap_handle)
{
	HANDLE wait_list[2];
	ULONG status;
	co_rc_t rc;
	
	co_win32_overlapped_init(&daemon_overlapped, daemon_handle);
	co_win32_overlapped_init(&tap_overlapped, tap_handle);

	wait_list[0] = tap_overlapped.read_event;
	wait_list[1] = daemon_overlapped.read_event;

	co_win32_overlapped_read_async(&tap_overlapped);
	co_win32_overlapped_read_async(&daemon_overlapped); 

	while (1) {
		status = WaitForMultipleObjects(2, wait_list, FALSE, INFINITE); 
			
		switch (status) {
		case WAIT_OBJECT_0: {/* tap */
			profile_me("from tap");
			rc = co_win32_overlapped_read_completed(&tap_overlapped);
			if (!CO_OK(rc))
				return 0;
			break;
		}
		case WAIT_OBJECT_0+1: {/* daemon */
			profile_me("from linux");
			rc = co_win32_overlapped_read_completed(&daemon_overlapped);
			if (!CO_OK(rc))
				return 0;

			break;
		}
		default:
			break;
		}
	}

	return 0;
}

/********************************************************************************
 * parameters
 */

void co_net_syntax()
{
	co_terminal_print("Cooperative Linux Virtual Network Daemon\n");
	co_terminal_print("Dan Aloni, 2004 (c)\n");
	co_terminal_print("\n");
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("  colinux-net-daemon -c 0 -i index [-h] [-n 'adapter name']\n");
	co_terminal_print("\n");
	co_terminal_print("    -h                      Show this help text\n");
	co_terminal_print("    -n 'adapter name'       The name of the network adapter to attach to\n");
	co_terminal_print("                            Without this option, the daemon tries to\n");
	co_terminal_print("                            guess which interface to use\n");
	co_terminal_print("    -i index                Network device index number (0 for eth0, 1 for\n");
	co_terminal_print("                            eth1, etc.)\n");
	co_terminal_print("    -c instance             coLinux instance ID to connect to\n");
}

static co_rc_t 
handle_paramters(start_parameters_t *start_parameters, int argc, char *argv[])
{
	char **param_scan = argv;
	const char *option;

	/* Default settings */
	start_parameters->index = -1;
	start_parameters->instance = -1;
	start_parameters->show_help = PFALSE;
	start_parameters->name_specified = PFALSE;

	/* Parse command line */
	while (*param_scan) {
		option = "-i";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("Parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			sscanf(*param_scan, "%d", &start_parameters->index);
			param_scan++;
			continue;
		}

		option = "-c";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("Parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			sscanf(*param_scan, "%d", (int *)&start_parameters->instance);
			param_scan++;
			continue;
		}

		option = "-n";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("Parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			snprintf(start_parameters->interface_name, 
				 sizeof(start_parameters->interface_name), 
				 "%s", *param_scan);

			start_parameters->name_specified = PTRUE;
			param_scan++;
			continue;
		}

		option = "-h";
		if (strcmp(*param_scan, option) == 0) {
			start_parameters->show_help = PTRUE;
		}

		param_scan++;
	}

	if (start_parameters->index == -1) {
		co_terminal_print("Device index not specified\n");
		return CO_RC(ERROR);
	}

	if ((start_parameters->index < 0) ||
	    (start_parameters->index >= CO_MODULE_MAX_CONET)) 
	{
		co_terminal_print("Invalid index: %d\n", start_parameters->index);
		return CO_RC(ERROR);
	}

	if (start_parameters->instance == -1) {
		co_terminal_print("coLinux instance not specificed\n");
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
	bool_t ret;
	HANDLE daemon_handle = 0;
	HANDLE tap_handle;
	int exit_code = 0;
	co_daemon_handle_t daemon_handle_;
	start_parameters_t start_parameters;
	char *prefered_name = NULL;

	co_debug_start();

	co_set_terminal_print_hook(terminal_print_hook_func);

	rc = handle_paramters(&start_parameters, argc, argv);
	if (!CO_OK(rc)) {
		exit_code = -1;
		goto out;
	}

	daemon_parameters = &start_parameters;

	prefered_name = daemon_parameters->name_specified ? daemon_parameters->interface_name : NULL;
	if (prefered_name == NULL) {
		co_terminal_print("auto selecting TAP\n");
 	} else {
		co_terminal_print("searching TAP device named \"%s\"\n", prefered_name);
	}

	rc = open_tap_win32(&tap_handle, prefered_name);
	if (!CO_OK(rc)) {
		if (CO_RC_GET_CODE(rc) == CO_RC_NOT_FOUND) {
			co_terminal_print("TAP device not found\n");
		} else {
			co_terminal_print("error opening TAP device (%x)\n", rc);
		}
		exit_code = -1;
		goto out;
	}

	co_terminal_print("enabling TAP...\n");

	ret = tap_win32_set_status(tap_handle, TRUE);
	if (!ret) {
		co_terminal_print("error enabling TAP Win32\n");
		exit_code = -1;
		goto out_close;

	}

	rc = co_os_daemon_pipe_open(daemon_parameters->instance, 
				    CO_MODULE_CONET0 + daemon_parameters->index, &daemon_handle_);
	if (!CO_OK(rc)) {
		co_terminal_print("Error opening a pipe to the daemon\n");
		goto out_close;
	}

	daemon_handle = daemon_handle_->handle;
	exit_code = wait_loop(daemon_handle, tap_handle);
	co_os_daemon_pipe_close(daemon_handle_);

out_close:
	CloseHandle(tap_handle);

out:
	co_debug_end();
	return exit_code;
}
