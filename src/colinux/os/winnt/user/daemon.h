/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

/*
 * Ballard, Jonathan H.  <jhballard@hotmail.com>
 *  20040222 : Redesigned co_daemon_handle for use with co_os_daemon_thread()
 *  20050117 : added packets, qFirst & qLast to co_daemon_handle
 */

#ifndef __COLINUX_USER_WINNT_DAEMON_H__
#define __COLINUX_USER_WINNT_DAEMON_H__

#include <windows.h>

#include <colinux/os/user/daemon.h>

struct co_daemon_handle {
	HANDLE handle;
	HANDLE thread;
	HANDLE readable;
	HANDLE shifted;
	HANDLE heap;
	co_message_t *message;
	void *qOut;
	void *qIn;
	void *packets;
	co_rc_t rc;
	bool_t loop;
};

#endif
