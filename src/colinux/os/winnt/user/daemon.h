/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */ 

#ifndef __COLINUX_USER_WINNT_DAEMON_H__
#define __COLINUX_USER_WINNT_DAEMON_H__

#include <windows.h>

#include <colinux/os/user/daemon.h>

#define CO_DAEMON_PIPE_BUFFER_SIZE 0x10000

struct co_daemon_handle {
	HANDLE handle;
	char *read_buffer;
	char *read_ptr;
	char *read_ptr_end;
	OVERLAPPED read_overlap;
	unsigned long read_size;
	bool_t read_pending;
};


#endif
