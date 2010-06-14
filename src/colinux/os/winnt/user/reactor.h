/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_WINNT_USER_REACTOR_H__
#define __COLINUX_WINNT_USER_REACTOR_H__

#include <colinux/user/reactor.h>

struct co_reactor_os_user {
	HANDLE read_event;
	HANDLE write_event;
	bool_t read_enabled;
	bool_t write_enabled;

	co_rc_t (*read)(co_reactor_user_t user);
	void (*write)(co_reactor_user_t user);
};

struct co_winnt_reactor_packet_user {
	struct co_reactor_user user;
	struct co_reactor_os_user os_user;
	HANDLE whandle;
	HANDLE rhandle;
	OVERLAPPED read_overlapped;
	OVERLAPPED write_overlapped;
	unsigned char buffer[0x10000];
	unsigned long size;
};

typedef struct co_winnt_reactor_packet_user *co_winnt_reactor_packet_user_t;

extern co_rc_t co_winnt_reactor_packet_user_create(
	co_reactor_t reactor, HANDLE whandle, HANDLE rhandle,
	co_reactor_user_receive_func_t receive,
	co_winnt_reactor_packet_user_t *handle_out);

extern void co_winnt_reactor_packet_user_destroy(
	co_winnt_reactor_packet_user_t user);

#endif
