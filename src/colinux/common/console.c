/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "console.h"

#include <colinux/os/alloc.h>

/*
 * Console initialization.
 */
static void blank_char(co_console_cell_t *cell)
{
	cell->ch = 0x20;    /* space */
	cell->attr = 0x07;  /* white on black */
}

co_rc_t co_console_create(long x, long y, long max_blacklog, co_console_t **console_out)
{
	unsigned long struct_size = 0;
	co_console_t *console;

	/*
	 * Use only one allocation for the entire console object so it would 
	 * be more managable (and less code).
	 */
	struct_size += sizeof(co_console_t);
	struct_size += sizeof(co_console_cell_t)*(y + max_blacklog)*x;

	console = (typeof(console))(co_os_malloc(struct_size));
	if (console == NULL)
		return CO_RC(ERROR);

	memset(console, 0, struct_size);

	console->x = x;
	console->y = y;
	console->size = struct_size;
	console->screen = ((co_console_cell_t *)((char *)console + sizeof(co_console_t)));
	console->backlog = console->screen + y*x;
	console->max_blacklog = max_blacklog;

	*console_out = console;

	return CO_RC(OK);
}

void co_console_destroy(co_console_t *console)
{
	co_os_free(console);
}

/*
 * Console message processor. 
 *
 * These messages come from the cocon driver of the coLinux kernel.
 */
co_rc_t co_console_op(co_console_t *console, co_console_message_t *message)
{
	switch (message->type) 
	{
	case CO_OPERATION_CONSOLE_SCROLL: {
		unsigned long t = message->scroll.t; 
		unsigned long b = message->scroll.b;
		unsigned long dir = message->scroll.dir;
		unsigned long lines = message->scroll.lines;
		unsigned long x, y;

		if (b > console->y)
			return CO_RC(ERROR);

		if (t + lines >= console->y)
			return CO_RC(ERROR);

		if (dir == 1) {
			memmove(&console->screen[console->x*t],
				&console->screen[console->x*(t + lines)],
				console->x*(b - t - lines)*sizeof(co_console_cell_t));

			for (y=b-lines; y < b; y++)
				for (x=0; x < console->x; x++)
					blank_char(&console->screen[y*console->x + x]);
		}
		else {
			memmove(&console->screen[console->x*(t + lines)],
				&console->screen[console->x*(t)],
				console->x*(b - t - lines)*sizeof(co_console_cell_t));

			for (y=t; y < t+lines; y++)
				for (x=0; x < console->x; x++)
					blank_char(&console->screen[y*console->x + x]);
		}

		break;
	}
	case CO_OPERATION_CONSOLE_PUTCS: {
		int x = message->putcs.x, y = message->putcs.y;
		int count = message->putcs.count;
		co_console_cell_t *cells = (co_console_cell_t *)&message->putcs.data;

		if (y >= console->y  ||  x >= console->x)
			return CO_RC(ERROR);

		while (x < console->x  &&  count > 0) {
			console->screen[y*console->x + x] = *cells;
			cells++;
			x++;
			count--;
		}
		break;
	}
	case CO_OPERATION_CONSOLE_PUTC: {
		int x = message->putc.x, y = message->putc.y;

		if (y >= console->y  ||  x >= console->x)
			return CO_RC(ERROR);
		
		console->screen[y*console->x + x] = 
			*(co_console_cell_t *)(&message->putc.charattr);
		break;
	}
	}

	return CO_RC(OK);
}

void co_console_pickle(co_console_t *console)
{
	((char *)console->screen) -= (unsigned long)console;
	((char *)console->backlog) -= (unsigned long)console;
}

void co_console_unpickle(co_console_t *console)
{
	((char *)console->screen) += ((unsigned long)console);
	((char *)console->backlog) += ((unsigned long)console);
}
