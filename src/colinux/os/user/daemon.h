/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
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

extern co_rc_t co_os_daemon_pipe_open(co_id_t linux_id, co_module_t module_id, co_daemon_handle_t *handle_out);
extern co_rc_t co_os_daemon_pipe_close(co_daemon_handle_t handle);
extern co_rc_t co_os_daemon_message_receive(co_daemon_handle_t handle, co_message_t **message, unsigned long timeout);
extern co_rc_t co_os_daemon_message_send(co_daemon_handle_t handle, co_message_t *message);
extern co_rc_t co_os_daemon_message_deallocate(co_daemon_handle_t handle, co_message_t *message);
extern void co_daemon_debug(char *str);
extern void co_daemon_print_header(void);

#endif
