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

#define NOISY 0

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
	case CO_OPERATION_CONSOLE_STARTUP:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_STARTUP\n");
#endif
		break;
	case CO_OPERATION_CONSOLE_INIT:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_INIT\n");
#endif
		break;
	case CO_OPERATION_CONSOLE_DEINIT:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_DEINIT\n");
#endif
		break;
	case CO_OPERATION_CONSOLE_PUTC:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_PUTC\n");
#endif
		return op_putc(message->putc.y, message->putc.x,
			       message->putc.charattr);

	case CO_OPERATION_CONSOLE_PUTCS:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_PUTCS\n");
#endif
		return op_putcs(message->putcs.y, message->putcs.x,
				message->putcs.data, message->putcs.count);

	case CO_OPERATION_CONSOLE_SCROLL_UP:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_SCROLL_UP\n");
#endif
		return op_scroll_up(message->scroll.top, message->scroll.bottom,
				 message->scroll.lines);
	case CO_OPERATION_CONSOLE_SCROLL_DOWN:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_SCROLL_DOWN\n");
#endif
		return op_scroll_down(message->scroll.top, message->scroll.bottom,
				 message->scroll.lines);

	case CO_OPERATION_CONSOLE_CURSOR_DRAW:
	case CO_OPERATION_CONSOLE_CURSOR_MOVE:
	case CO_OPERATION_CONSOLE_CURSOR_ERASE:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_CURSOR_\n");
#endif
		return op_cursor(message->cursor);

	case CO_OPERATION_CONSOLE_CLEAR:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_CLEAR\n");
#endif
		return op_clear(message->clear.top, message->clear.left,
				message->clear.bottom, message->clear.right,
			        message->clear.charattr);
	case CO_OPERATION_CONSOLE_BMOVE:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_BMOVE\n");
#endif
		return op_bmove(message->bmove.row, message->bmove.column,
				message->bmove.top, message->bmove.left,
				message->bmove.bottom, message->bmove.right);
	case CO_OPERATION_CONSOLE_SWITCH:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_SWITCH\n");
#endif
		break;
	case CO_OPERATION_CONSOLE_BLANK:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_BLANK\n");
#endif
		break;
	case CO_OPERATION_CONSOLE_FONT_OP:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_FONT_OP\n");
#endif
		break;
	case CO_OPERATION_CONSOLE_SET_PALETTE:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_SET_PALETTE\n");
#endif
		break;
	case CO_OPERATION_CONSOLE_SCROLLDELTA:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_SCROLLDELTA\n");
#endif
		break;
	case CO_OPERATION_CONSOLE_SET_ORIGIN:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_SET_ORIGIN\n");
#endif
		break;
	case CO_OPERATION_CONSOLE_SAVE_SCREEN:
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_SAVE_SCREEN\n");
#endif
		break;
	case CO_OPERATION_CONSOLE_INVERT_REGION:
		return op_invert(message->invert.y, message->invert.x,
				 message->invert.count);
#if NOISY
		co_debug("CO_OPERATION_CONSOLE_INVERT_REGION\n");
#endif
		break;
	default:
		co_debug("CO_OPERATION_CONSOLE_%d\n", message->type);
	}

	return CO_RC(ERROR);
}
