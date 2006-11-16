/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __COLINUX_LINUX_USER_FRAME_H__
#define __COLINUX_LINUX_USER_FRAME_H__

#include <colinux/common/common.h>

#include "unix.h"

co_rc_t co_os_frame_send(int sock, char *data, unsigned long size);

typedef struct {
	char *buffer;
	unsigned long size;
	unsigned long buffer_received;
	unsigned long max_buffer_size;
} co_os_frame_collector_t;

co_rc_t co_os_frame_recv(co_os_frame_collector_t *frame_collector, 
			 int sock, char **data, unsigned long *size);

#endif
