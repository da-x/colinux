/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "console.h"

#include <memory.h>

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
	case CO_OPERATION_CONSOLE_SCROLL_UP:
	case CO_OPERATION_CONSOLE_SCROLL_DOWN: {
		unsigned long t = message->scroll.top;
		unsigned long b = message->scroll.bottom+1;
		unsigned long dir;
		unsigned long lines = message->scroll.lines;
		unsigned long x, y;

		if(message->type==CO_OPERATION_CONSOLE_SCROLL_UP)
			dir = 1;
		else
			dir = 2;

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
	case CO_OPERATION_CONSOLE_CURSOR_DRAW:
	case CO_OPERATION_CONSOLE_CURSOR_ERASE:
	case CO_OPERATION_CONSOLE_CURSOR_MOVE: {
		console->cursor = message->cursor;
		break;
	}

	case CO_OPERATION_CONSOLE_CLEAR:{
		unsigned t = message->clear.top;
		unsigned l ;
		unsigned b = message->clear.bottom;
		unsigned r = message->clear.right;
		co_console_cell_t *cell;

		while(t<=b) {
			l = message->clear.left;
			cell = &console->screen[t*console->x+l];
			while(l++<=r) {
				cell->attr = 0x07;
				(cell++)->ch = ' ';
			}
			t++;
		}
		break;
	}

	case CO_OPERATION_CONSOLE_BMOVE:{
		unsigned y = message->bmove.row;
		unsigned x = message->bmove.column;
		unsigned t = message->bmove.top;
		unsigned l = message->bmove.left;
		unsigned b = message->bmove.bottom;
		unsigned r = message->bmove.right;

		if(y<t) {
			while(t<=b) {
				memmove(&console->screen[y*console->x+x],
					&console->screen[t*console->x+l],
					r-l+1);
				t++;
				y++;
			}
		} else	{
			y+=b-t;
			while(t<=b) {
				memmove(&console->screen[y*console->x+x],
					&console->screen[b*console->x+l],
					r-l+1);
				b--;
				y--;
			}
		}
	}

	case CO_OPERATION_CONSOLE_STARTUP:
	case CO_OPERATION_CONSOLE_INIT:
	case CO_OPERATION_CONSOLE_DEINIT:
	case CO_OPERATION_CONSOLE_SWITCH:
	case CO_OPERATION_CONSOLE_BLANK:
	case CO_OPERATION_CONSOLE_FONT_OP:
	case CO_OPERATION_CONSOLE_SET_PALETTE:
	case CO_OPERATION_CONSOLE_SCROLLDELTA:
	case CO_OPERATION_CONSOLE_SET_ORIGIN:
	case CO_OPERATION_CONSOLE_SAVE_SCREEN:
	case CO_OPERATION_CONSOLE_INVERT_REGION:
		break;
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

