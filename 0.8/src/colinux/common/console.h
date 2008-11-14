/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_COMMON_CONSOLE__
#define __COLINUX_COMMON_CONSOLE__

#include "common.h"

/*
 * This is a generic console implementation. It receives 'messages'
 * about what happens in the console and modifies the console buffer
 * accordingly. This 'object' can be toggled to be relocatable using
 * the pickle/unpickle functions. It can be used to copy this object
 * from and to userspace (i.e, when attaching and detaching the
 * coLinux console).
 *
 * The type of console messages are defined in linux/cooperative.h 
 * (as it is needed by the Linux code).
 */

#define CO_CONSOLE_WIDTH      80
#define CO_CONSOLE_HEIGHT     25
#define CO_CONSOLE_HEIGHT_BUF  0

typedef struct co_console_cell { 
	unsigned char ch;
	unsigned char attr;
} co_console_cell_t;

typedef struct co_console {
	/* size of this struct */
	unsigned long size;

	/* dimentions of the console */
	long x, y;

	/* On-screen data */
	co_console_cell_t *screen;

	/* <not yet implemented> */
	/* Off-screen (scrolled data) */
	co_console_cell_t *backlog;
	long max_blacklog;
	long used_blacklog;
	long last_blacklog;
	/* </not yet implemented> */

	/* Cursor pos */
	co_cursor_pos_t cursor;

	/* Size of the cursor (0-empty, 255-full) */
	unsigned char cursor_size;
} co_console_t;

extern co_rc_t co_console_create(long x, long y, long max_blacklog, co_console_t **console_out);
extern co_rc_t co_console_op(co_console_t *console, co_console_message_t *message);
extern void co_console_destroy(co_console_t *console);
extern void co_console_pickle(co_console_t *console);
extern void co_console_unpickle(co_console_t *console);

#endif
