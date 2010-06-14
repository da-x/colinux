/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_USER_REACTOR_H__
#define __COLINUX_USER_REACTOR_H__

#include <colinux/common/common.h>
#include <colinux/common/ioctl.h>
#include <colinux/common/list.h>

struct co_reactor_os_user;
typedef struct co_reactor_os_user *co_reactor_os_user_t;

struct co_reactor_user;
typedef struct co_reactor_user *co_reactor_user_t;

typedef struct co_reactor *co_reactor_t;

typedef co_rc_t (*co_reactor_user_receive_func_t)(co_reactor_user_t user, unsigned char *buffer, unsigned long size);
typedef co_rc_t (*co_reactor_user_send_func_t)(co_reactor_user_t user, unsigned char *buffer, unsigned long size);

struct co_reactor_user {
	co_list_t node;
	co_reactor_os_user_t os_data;
	void *private_data;
	co_reactor_t reactor;
	co_rc_t last_read_rc;

	co_reactor_user_receive_func_t received;
	co_reactor_user_send_func_t send;
};

struct co_reactor {
	co_list_t users;
	unsigned long num_users;
};

extern co_rc_t co_reactor_create(co_reactor_t *out_handle);
extern co_rc_t co_reactor_select(co_reactor_t handle, int miliseconds);
extern void co_reactor_add(co_reactor_t handle, co_reactor_user_t user);
extern void co_reactor_remove(co_reactor_user_t user);
extern void co_reactor_destroy(co_reactor_t handle);

#endif
