/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __COLINUX_USER_CONSOLE_DAEMON_H__
#define __COLINUX_USER_CONSOLE_DAEMON_H__

typedef enum {
	CO_DAEMON_CONSOLE_MESSAGE_ATTACH,
	CO_DAEMON_CONSOLE_MESSAGE_DETACH,
	CO_DAEMON_CONSOLE_MESSAGE_TERMINATE,
} co_daemon_console_message_type_t;

typedef struct {
	co_daemon_console_message_type_t type;
	unsigned long size;
	char data[];
} co_daemon_console_message_t;

#endif
