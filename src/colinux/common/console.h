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

#ifndef __COLINUX_LINUX_CONFIG_H__ /* Exclude circullar "include" */
# include "config.h" 
#endif


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

#define CO_CONSOLE_WIDTH	80
#define CO_CONSOLE_HEIGHT	25

#define CO_CONSOLE_HEIGHT_BUF	0

/* Cursor size */
#define CO_CONSOLE_NORM_CURSOR	10
  /* Normal cursor (one scanline) */

#define CO_CONSOLE_FAT_CURSOR	99
  /* "Fat" cursor (max possible size) */

typedef struct co_console_cell { 
	unsigned char ch;
	unsigned char attr;
} co_console_cell_t;

#if CO_ENABLE_COCON_MAX_LINES || CO_ENABLE_SCROLL_BUF
/* Set memory limit for console buffer up to 16 * 32 KB with scrolling */
# define CO_CONSOLE_MAX_CHARS	((16 * 32 * 1024) / sizeof(co_console_cell_t))
#else
/* Set memory limit for console buffer up to 32 KB without scrolling */
# define CO_CONSOLE_MAX_CHARS	((1  * 32 * 1024) / sizeof(co_console_cell_t))
#endif

typedef struct co_console {
	/* size of this struct */
	unsigned long	size;

	/* dimentions of the console */
	long		x;
	long		y;
	long		max_y;

	/* On-screen data */
	co_console_cell_t* screen;

	/* <not yet implemented> */
	/* Off-screen (scrolled data) */
	co_console_cell_t* backlog;
	long		   used_blacklog;
	long		   last_blacklog;
	/* </not yet implemented> */

	/* Cursor position and height in percent
	 * Defined in 'linux/cooperative.h' as x, y, height.
	 */
	co_cursor_pos_t cursor;
} co_console_t;

extern co_rc_t co_console_create(long		x,
			  	 long	        y,
			  	 long		max_y,
			  	 int		curs_size_prc,
			  	 co_console_t** console_out);
  /* Create the console object.
     Parameters:
       x 	     - maximum height of the visible part of the console 
       y 	     - maximum width of the visible part of the console 
       max_y         - maximum number of rows in the screen buffer
       curs_size_prc - initial size of the cursor in percent
  */
		                 
extern co_rc_t co_console_op(co_console_t* console, co_console_message_t* message);
extern void co_console_destroy(co_console_t* console);

#if defined UNUSED
extern void co_console_pickle(co_console_t* console);
extern void co_console_unpickle(co_console_t* console);
#endif

#endif
