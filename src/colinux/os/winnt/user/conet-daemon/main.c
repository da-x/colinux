/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <windows.h>
#include <stdio.h>

#include <colinux/common/common.h>

#include "tap-win32.h"
#include "../daemon.h"

/*
 * IMPORTANT NOTE:
 *
 * This is work-in-progress. This daemon is currently hardcoded
 * to work against coLinux0 as conet0. Expect changes.
 *
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

co_win32_overlapped_t tap_overlapped, daemon_overlapped;

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

		message = (co_message_t *)overlapped->buffer;

		co_win32_overlapped_write_async(&tap_overlapped, message->data, message->size);

	} else {
		/* Received packet from TAP */
		struct {
			co_message_t message;
			co_linux_message_t linux;
			char data[overlapped->size];
		} message;
		
		message.message.from = CO_MODULE_CONET0;
		message.message.to = CO_MODULE_LINUX;
		message.message.priority = CO_PRIORITY_DISCARDABLE;
		message.message.type = CO_MESSAGE_TYPE_OTHER;
		message.message.size = sizeof(message) - sizeof(message.message);
		message.linux.device = CO_DEVICE_NETWORK;
		message.linux.unit = 0;
		message.linux.size = overlapped->size;
		memcpy(message.data, overlapped->buffer, overlapped->size);
		
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
			rc = co_win32_overlapped_read_completed(&tap_overlapped);
			if (!CO_OK(rc))
				return 0;
			break;
		}
		case WAIT_OBJECT_0+1: {/* daemon */
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

int main(int argc, char *argv[])
{	
	co_rc_t rc;
	bool_t ret;
	HANDLE daemon_handle = 0;
	HANDLE tap_handle;
	int exit_code = 0;
	co_daemon_handle_t daemon_handle_;

	rc = open_tap_win32(&tap_handle);
	if (!CO_OK(rc)) {
		co_debug("Error opening TAP Win32\n");
		exit_code = -1;
		goto out;
	}

	co_debug("Enabling TAP-Win32...\n");

	ret = tap_win32_set_status(tap_handle, TRUE);
	if (!ret) {
		co_debug("Error enabling TAP Win32\n");
		exit_code = -1;
		goto out_close;

	}

	rc = co_os_open_daemon_pipe(0, CO_MODULE_CONET0, &daemon_handle_);
	if (!CO_OK(rc)) {
		co_debug("Error opening a pipe to the daemon\n");
		goto out_close;
	}

	daemon_handle = daemon_handle_->handle;
	exit_code = wait_loop(daemon_handle, tap_handle);
	co_os_daemon_close(daemon_handle_);

out_close:
	CloseHandle(tap_handle);

out:
	return exit_code;
}
