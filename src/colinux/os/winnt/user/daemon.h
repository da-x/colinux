/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

/*
 * Ballard, Jonathan H.  <californiakidd@users.sourceforge.net>
 *  2004 02 22	: Redesigned co_daemon_handle for use with co_os_daemon_thread()
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
	co_message_t *message;
	co_rc_t rc;
	bool_t loop;
};

#define CO_OS_DAEMON_QUEUE_MINIMUM		0x100

#endif
