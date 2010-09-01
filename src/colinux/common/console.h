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

#ifndef __COLINUX_LINUX_CONFIG_H__ /* Exclude circullar "include" */
# include "config.h"
#endif

#if defined __cplusplus
extern "C" {
#endif

/* Set lines limit for console buffer in 2 ... 500 */
#define CO_CONSOLE_MIN_ROWS		2
#define CO_CONSOLE_MAX_ROWS		500

#define CO_CONSOLE_MIN_COLS		16

/* Default console screen dimentions */
#define CO_CONSOLE_WIDTH		80
#define CO_CONSOLE_HEIGHT		25

/* Cursor size */
/* Definitions for cursor.height Taken from Linux console_struct.h */
#define CO_CUR_DEF		0	/* User defined */
#define CO_CUR_NONE		1	/* 0 */
#define CO_CUR_UNDERLINE	2	/* 1/6 */
#define CO_CUR_LOWER_THIRD	3	/* 1/3 */
#define CO_CUR_LOWER_HALF	4	/* 2/2 */
#define CO_CUR_TWO_THIRDS	5	/* 2/3 */
#define CO_CUR_BLOCK		6	/* 1 */

#define CO_CUR_DEFAULT		CO_CUR_UNDERLINE /* default */

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

typedef struct co_console_cell {
	unsigned char ch;
	unsigned char attr;
} co_console_cell_t;

typedef struct co_console {
	/* size of this struct */
	unsigned long	size;

        /* User defined configuration */
	co_console_config_t config;

	/* On-screen data */
	co_console_cell_t* screen;

	/* back log buffer */
	co_console_cell_t* buffer;

	/* Cursor position and height in percent
	 * Defined in 'linux/cooperative.h' as x, y, height.
	 */
	co_cursor_pos_t cursor;
} co_console_t;

extern co_rc_t co_console_create(co_console_config_t* config_p,
			  	 co_console_t**       console_out);
  /* Create the console object.
  */

extern co_rc_t co_console_op(co_console_t* console, co_console_message_t* message);
extern void co_console_destroy(co_console_t* console);

#if defined UNUSED
extern void co_console_pickle(co_console_t* console);
extern void co_console_unpickle(co_console_t* console);
#endif

#if defined __cplusplus
}
#endif

#endif /* __COLINUX_COMMON_CONSOLE__ */
