/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "console.h"
#include "widget.h"

extern "C" {
#include <colinux/os/alloc.h>
} console_widget_t::console_widget_t()
{
	console = 0;
}

console_widget_t::~console_widget_t()
{
}

void
 console_widget_t::redraw()
{
	draw();
}

void
 console_widget_t::co_console(co_console_t * _console)
{
	console = _console;
}

co_console_t *
console_widget_t::co_console()
{
	co_console_t *_console = console;
	console = 0;

	return _console;
}

co_rc_t console_widget_t::event(co_console_message_t * message)	// UPDATE: & message
{
	co_rc_t
	    rc;

	if (!console) {
		return CO_RC(ERROR);
	}
	rc = co_console_op(console, message);
	if (!CO_OK(rc))
		return rc;

	switch (message->type) {
	case CO_OPERATION_CONSOLE_PUTC:
		return op_putc(message->putc.y, message->putc.x,
			       message->putc.charattr);

	case CO_OPERATION_CONSOLE_PUTCS:
		return op_putcs(message->putcs.y, message->putcs.x,
				message->putcs.data, message->putcs.count);

	case CO_OPERATION_CONSOLE_SCROLL:
		return op_scroll(message->scroll.t, message->scroll.b,
				 message->scroll.dir, message->scroll.lines);

	case CO_OPERATION_CONSOLE_CURSOR:
		return op_cursor(message->cursor);
	}

	return CO_RC(ERROR);
}
