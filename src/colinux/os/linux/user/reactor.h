/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_LINUX_USER_REACTOR_H__
#define __COLINUX_LINUX_USER_REACTOR_H__

#include <colinux/user/reactor.h>

struct co_reactor_os_user {
	int fd;

	co_rc_t (*read)(co_reactor_user_t user);
	void (*write)(co_reactor_user_t user);
};

struct co_linux_reactor_packet_user {
	struct co_reactor_user user;
	struct co_reactor_os_user os_user;

	unsigned char buffer[0x10000];
	unsigned long size;
};

typedef struct co_linux_reactor_packet_user *co_linux_reactor_packet_user_t;

extern co_rc_t co_linux_reactor_packet_user_create(
	co_reactor_t reactor, int fd,
	co_reactor_user_receive_func_t receive,
	co_linux_reactor_packet_user_t *handle_out);

extern void co_linux_reactor_packet_user_destroy(
	co_linux_reactor_packet_user_t user);

#endif
