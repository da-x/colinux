/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_DAEMON_H__
#define __COLINUX_OS_USER_DAEMON_H__

#include <colinux/common/common.h>

struct co_daemon_handle;
typedef struct co_daemon_handle *co_daemon_handle_t; 

extern co_rc_t co_os_open_daemon_pipe(co_id_t linux_id, co_module_t module_id, co_daemon_handle_t *handle_out);
extern co_rc_t co_os_daemon_get_message(co_daemon_handle_t handle, 
					co_message_t **message,
					unsigned long timeout);
extern co_rc_t co_os_daemon_send_message(co_daemon_handle_t handle, co_message_t *message);
extern void co_os_daemon_close(co_daemon_handle_t handle);

#endif
