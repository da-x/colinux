/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __CO_WIN32_NETWORK_H__
#define __CO_WIN32_NETWORK_H__

#include <windows.h>
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>

typedef struct {
	co_rc_t id;

	HANDLE tap_handle;

	HANDLE tap_read_event;
	HANDLE tap_write_event;
	HANDLE tap_cancel_event;

	HANDLE monitor_event;
	HANDLE monitor_thread_continue;

	HANDLE monitor_thread_finish;
	HANDLE tap_thread_finish;

	OVERLAPPED tap_read_overlapped;
	OVERLAPPED tap_write_overlapped;

	co_user_monitor_t *monitor;
	long running;

	struct {
		co_monitor_ioctl_network_receive_t header;
		char data[1600];
	} receive_packet;
	struct {
		co_manager_ioctl_monitor_t pc;
		char data[1600];
		unsigned long size;
	} send_packet;
} conet_daemon_t;

co_rc_t conet_daemon_start(co_rc_t id, conet_daemon_t **daemon_out);
void conet_daemon_stop(conet_daemon_t *daemon);
co_rc_t conet_test_tap();

#endif
