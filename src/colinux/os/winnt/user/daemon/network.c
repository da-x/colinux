/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <windows.h>
#include <stdio.h>

#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/os/alloc.h>

#include "network.h"
#include "win32tap.h"

DWORD WINAPI conet_monitor_thread(DWORD param)
{
	conet_daemon_t *daemon;
	co_user_monitor_t *monitor;
	co_rc_t rc;

	daemon = (typeof(daemon))param;

	rc = co_user_monitor_open(daemon->id, &monitor);
	if (!CO_OK(rc)) {
		printf("Error opening coLinux driver\n");
		rc = CO_RC(ERROR);
		goto out;
	}
	
	while (daemon->running) {
		rc = co_user_monitor_network_poll(monitor);
		if (!CO_OK(rc))
			break;

		SetEvent(daemon->monitor_event);
	}

out:
	co_user_monitor_close(monitor);

	SetEvent(daemon->monitor_thread_finish);

	return 0;
}

co_rc_t conet_host_to_linux(conet_daemon_t *daemon)
{
	co_rc_t rc;

	rc = co_user_monitor_network_send(daemon->monitor, 
					  (char *)&daemon->send_packet.pc,
					  sizeof(co_manager_ioctl_monitor_t) +
					  daemon->send_packet.size);
	
	return rc;
}

co_rc_t conet_linux_to_host(conet_daemon_t *daemon)
{
	unsigned long write_size = 0;
	BOOL result;
	DWORD error;

	while (1) {
		co_rc_t rc;
		
		rc = co_user_monitor_network_receive(daemon->monitor, 
						     (char *)&daemon->receive_packet,
						     sizeof(daemon->receive_packet));
		if (!CO_OK(rc))
			break;
		
		result = WriteFile(daemon->tap_handle, 
				   daemon->receive_packet.data,
				   daemon->receive_packet.header.size, 
				   &write_size, &daemon->tap_write_overlapped);

		if (!result) { 
			switch (error = GetLastError())
			{ 
			case ERROR_IO_PENDING: 
				WaitForSingleObject(daemon->tap_write_event, INFINITE);
				break;
			default:
				co_debug("%d\n", error);
				return CO_RC(ERROR);
			} 
		}

		if (daemon->receive_packet.header.more == 0)
			break;
	}

	return CO_RC(OK);
}

co_rc_t conet_read_async(conet_daemon_t *daemon)
{
	BOOL result;
	DWORD error;
	unsigned long read_size;

	while (TRUE) {
		result = ReadFile(daemon->tap_handle,
				  &daemon->send_packet.data,
				  sizeof(daemon->send_packet.data),
				  &read_size,
				  &daemon->tap_read_overlapped);

		if (!result) { 
			switch (error = GetLastError())
			{ 
			case ERROR_IO_PENDING: 
				return CO_RC(OK);
			default:
				return CO_RC(ERROR);
			} 
		} else {
			daemon->send_packet.size = read_size;
			conet_host_to_linux(daemon);
		}
	}

	return CO_RC(OK);
}

co_rc_t conet_read_overlapped_completed(conet_daemon_t *daemon)
{
	unsigned long read_size;
	BOOL result;

	result = GetOverlappedResult(
		daemon->tap_handle,
		&daemon->tap_read_overlapped,
		&read_size,
		FALSE);

	if (result) {
		daemon->send_packet.size = read_size;
		conet_host_to_linux(daemon);
		conet_read_async(daemon);
	} else {
		printf("GetOverlappedResult error\n");
	}
	 
	return CO_RC(OK);
}

DWORD WINAPI conet_tap_thread(DWORD param)
{
	conet_daemon_t *daemon;
	co_user_monitor_t *monitor;
	co_rc_t rc;
	HANDLE events[3];
	HANDLE tap;
	BOOL bret;

	daemon = (typeof(daemon))param;

	rc = open_win32_tap(&daemon->tap_handle);
	if (!CO_OK(rc)) {
		printf("Error opening TAP driver (%d)\n", rc);
		goto out;
	}

	tap = daemon->tap_handle;

	rc = co_user_monitor_open(daemon->id, &monitor);
	if (!CO_OK(rc)) {
		printf("Error opening coLinux driver\n");
		rc = CO_RC(ERROR);
		goto out_close_tap;
	}

	bret = win32_tap_set_status(daemon->tap_handle, TRUE);

	conet_read_async(daemon);

	daemon->monitor = monitor;

	events[0] = daemon->monitor_event;
	events[1] = daemon->tap_read_event;
	events[2] = daemon->tap_cancel_event;

	while (TRUE) {
		DWORD result = WaitForMultipleObjects(3,  &events, FALSE, INFINITE);

		if (result == (WAIT_OBJECT_0)) {
			conet_linux_to_host(daemon);
		}
		else if (result == (WAIT_OBJECT_0 + 1)) {
			conet_read_overlapped_completed(daemon);
		} 
		else if (result == (WAIT_OBJECT_0 + 2)) {
			break;
		}

	}

	co_user_monitor_close(monitor);

	SetEvent(daemon->tap_thread_finish);

out_close_tap:
	CloseHandle(tap);

out:
	return 0;
}

co_rc_t conet_daemon_init(conet_daemon_t *daemon)
{
	HANDLE thread_handle;
	DWORD thread_id;

	daemon->tap_read_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	daemon->tap_write_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	daemon->tap_cancel_event = CreateEvent(NULL, FALSE, FALSE, NULL);

	daemon->monitor_event = CreateEvent(NULL, FALSE, FALSE, NULL);

	daemon->tap_thread_finish = CreateEvent(NULL, TRUE, FALSE, NULL);	
	daemon->monitor_thread_finish = CreateEvent(NULL, TRUE, FALSE, NULL);	

	daemon->tap_read_overlapped.Offset = 0; 
	daemon->tap_read_overlapped.OffsetHigh = 0; 
	daemon->tap_read_overlapped.hEvent = daemon->tap_read_event; 

	daemon->tap_write_overlapped.Offset = 0; 
	daemon->tap_write_overlapped.OffsetHigh = 0; 
	daemon->tap_write_overlapped.hEvent = daemon->tap_write_event; 

	thread_handle = CreateThread(
		NULL, 0, (LPTHREAD_START_ROUTINE)&conet_monitor_thread,
		(PVOID)daemon, 0, (PULONG) &thread_id);

	thread_handle = CreateThread(
		NULL, 0, (LPTHREAD_START_ROUTINE)&conet_tap_thread,
		(PVOID)daemon, 0, (PULONG) &thread_id);

	return CO_RC(OK);
}

co_rc_t conet_daemon_start(co_rc_t id, conet_daemon_t **daemon_out)
{
	conet_daemon_t *daemon;
	co_rc_t rc;

	daemon = co_os_malloc(sizeof(*daemon));
	if (daemon == NULL)
		return CO_RC(ERROR);

	memset(daemon, 0, sizeof(*daemon));
	
	daemon->id = id;
	daemon->running = 1;

	rc = conet_daemon_init(daemon);
	if (!CO_OK(rc)) {
		co_os_free(daemon);
		return rc;
	}

	*daemon_out = daemon;
	return rc;
}

void conet_daemon_stop(conet_daemon_t *daemon)
{
	daemon->running = 0;

	co_user_monitor_network_cancel_poll(daemon->monitor);
	SetEvent(daemon->tap_cancel_event);

	WaitForSingleObject(daemon->tap_thread_finish, INFINITE);
	WaitForSingleObject(daemon->monitor_thread_finish, INFINITE);

	co_os_free(daemon);
}
