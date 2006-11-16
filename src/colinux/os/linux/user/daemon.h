/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_LINUX_USER_DAEMON_H__
#define __COLINUX_LINUX_USER_DAEMON_H__

#include <colinux/os/user/daemon.h>

#include "frame.h"

struct co_daemon_handle {
	int sock;
	co_os_frame_collector_t frame;
};

co_rc_t
co_os_daemon_get_message_ready(co_daemon_handle_t handle,
			       co_message_t **message_out);

#endif
