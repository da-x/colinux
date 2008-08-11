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
} 

console_widget_t::console_widget_t()
{
	console = 0;
}

console_widget_t::~console_widget_t()
{
}

void console_widget_t::redraw()
{
	draw();
}

void console_widget_t::set_console(co_console_t * _console)
{
	console = _console;
}


#define DEBUG_CONSOLE 0

co_rc_t console_widget_t::event(co_console_message_t *message)
{
	co_rc_t rc;

	if (!console)
		return CO_RC(ERROR);

	rc = co_console_op(console, message);
	if (!CO_OK(rc))
		return rc;

	switch (message->type) {
	case CO_OPERATION_CONSOLE_STARTUP:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_STARTUP");
#endif
		break;
	case CO_OPERATION_CONSOLE_INIT:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_INIT");
#endif
		break;
	case CO_OPERATION_CONSOLE_DEINIT:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_DEINIT");
#endif
		break;
	case CO_OPERATION_CONSOLE_PUTC:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_PUTC");
#endif
		return op_putc(message->putc.y, message->putc.x,
			       message->putc.charattr);

	case CO_OPERATION_CONSOLE_PUTCS:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_PUTCS");
#endif
		return op_putcs(message->putcs.y, message->putcs.x,
				message->putcs.data, message->putcs.count);

	case CO_OPERATION_CONSOLE_SCROLL_UP:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_SCROLL_UP");
#endif
		return op_scroll_up(message->scroll.top, message->scroll.bottom,
				 message->scroll.lines);
	case CO_OPERATION_CONSOLE_SCROLL_DOWN:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_SCROLL_DOWN");
#endif
		return op_scroll_down(message->scroll.top, message->scroll.bottom,
				 message->scroll.lines);

	case CO_OPERATION_CONSOLE_CURSOR_DRAW:
	case CO_OPERATION_CONSOLE_CURSOR_MOVE:
	case CO_OPERATION_CONSOLE_CURSOR_ERASE:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_CURSOR_");
#endif
		return op_cursor(message->cursor);

	case CO_OPERATION_CONSOLE_CLEAR:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_CLEAR");
#endif
		return op_clear(message->clear.top, message->clear.left,
				message->clear.bottom, message->clear.right,
			        message->clear.charattr);
	case CO_OPERATION_CONSOLE_BMOVE:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_BMOVE");
#endif
		return op_bmove(message->bmove.row, message->bmove.column,
				message->bmove.top, message->bmove.left,
				message->bmove.bottom, message->bmove.right);
	case CO_OPERATION_CONSOLE_SWITCH:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_SWITCH");
#endif
		break;
	case CO_OPERATION_CONSOLE_BLANK:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_BLANK");
#endif
		break;
	case CO_OPERATION_CONSOLE_FONT_OP:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_FONT_OP");
#endif
		break;
	case CO_OPERATION_CONSOLE_SET_PALETTE:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_SET_PALETTE");
#endif
		break;
	case CO_OPERATION_CONSOLE_SCROLLDELTA:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_SCROLLDELTA");
#endif
		break;
	case CO_OPERATION_CONSOLE_SET_ORIGIN:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_SET_ORIGIN");
#endif
		break;
	case CO_OPERATION_CONSOLE_SAVE_SCREEN:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_SAVE_SCREEN");
#endif
		break;
	case CO_OPERATION_CONSOLE_INVERT_REGION:
		return op_invert(message->invert.y, message->invert.x,
				 message->invert.count);
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_INVERT_REGION");
#endif
		break;
	default:
		co_debug("CO_OPERATION_CONSOLE_%d", message->type);
	}

	return CO_RC(ERROR);
}
