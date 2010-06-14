/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

 /*
  * OS independent widget class
  * Used for building colinux-console-nt.exe
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

void console_widget_t::set_console(co_console_t* _console)
{
	console = _console;
}


#define DEBUG_CONSOLE 0

/* Process console messages:
	CO_OPERATION_CONSOLE_PUTC		- Put single char-attr pair
	CO_OPERATION_CONSOLE_PUTCS		- Put char-attr pair array

	CO_OPERATION_CONSOLE_CURSOR_MOVE	- Move cursor
	CO_OPERATION_CONSOLE_CURSOR_DRAW	- Move cursor
	CO_OPERATION_CONSOLE_CURSOR_ERASE	- Move cursor

	CO_OPERATION_CONSOLE_SCROLL_UP		- Scroll lines up
	CO_OPERATION_CONSOLE_SCROLL_DOWN	- Scroll lines down
	CO_OPERATION_CONSOLE_BMOVE		- Move region up/down

	CO_OPERATION_CONSOLE_CLEAR		- Clear region

	CO_OPERATION_CONSOLE_BLANK		- Ignored
	CO_OPERATION_CONSOLE_SWITCH		- Ignored
	CO_OPERATION_CONSOLE_FONT_OP		- Ignored
	CO_OPERATION_CONSOLE_SET_PALETTE	- Ignored
	CO_OPERATION_CONSOLE_SCROLLDELTA	- Ignored
	CO_OPERATION_CONSOLE_SET_ORIGIN		- Ignored
	CO_OPERATION_CONSOLE_SAVE_SCREEN	- Ignored
	CO_OPERATION_CONSOLE_INVERT_REGION	- Ignored
	CO_OPERATION_CONSOLE_CONFIG		- Ignored
	CO_OPERATION_CONSOLE_STARTUP		- Ignored
	CO_OPERATION_CONSOLE_INIT		- Ignored
	CO_OPERATION_CONSOLE_DEINIT		- Ignored
*/
co_rc_t console_widget_t::event(co_console_message_t* message)
{
	co_rc_t rc;

	if (!console)
		return CO_RC(ERROR);

	switch (message->type)
	{
	case CO_OPERATION_CONSOLE_STARTUP:
		// Workaround: Do not call co_console_op here. Size of message data
		// is 0 and has no space for call back data. This would destroy
		// io_buffer for next CO_OPERATION_CONSOLE_INIT_SCROLLBUFFER.
		return CO_RC(OK);
	default:
		break;
	}

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
		set_cursor_size(console->config.curs_type_size);
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
		return op_putc(message->putc.y,
			       message->putc.x,
			       message->putc.charattr);

	case CO_OPERATION_CONSOLE_PUTCS:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_PUTCS");
#endif
		return op_putcs(message->putcs.y,
				message->putcs.x,
				message->putcs.data,
				message->putcs.count);

	case CO_OPERATION_CONSOLE_SCROLL_UP:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_SCROLL_UP");
#endif
		return op_scroll_up(message->scroll.top,
				    message->scroll.bottom,
				    message->scroll.lines,
				    message->scroll.charattr);
	case CO_OPERATION_CONSOLE_SCROLL_DOWN:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_SCROLL_DOWN");
#endif
		return op_scroll_down(message->scroll.top,
				      message->scroll.bottom,
				      message->scroll.lines,
				      message->scroll.charattr);

	case CO_OPERATION_CONSOLE_BMOVE:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_BMOVE");
#endif
		return op_bmove(message->bmove.row,
				message->bmove.column,
				message->bmove.top,
				message->bmove.left,
				message->bmove.bottom,
				message->bmove.right);


	case CO_OPERATION_CONSOLE_CURSOR_DRAW:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_CURSOR_DRAW");
#endif
		if (message->cursor.height == CO_CUR_DEF)
			set_cursor_size(console->config.curs_type_size);
		else
			set_cursor_size(message->cursor.height);
		return op_cursor_pos(message->cursor);

	case CO_OPERATION_CONSOLE_CURSOR_ERASE:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_CURSOR_ERASE");
#endif
		return set_cursor_size(CO_CUR_NONE);

	case CO_OPERATION_CONSOLE_CURSOR_MOVE:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_CURSOR_MOVE");
#endif
		return op_cursor_pos(message->cursor);

	case CO_OPERATION_CONSOLE_CLEAR:
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_CLEAR");
#endif
		return op_clear(message->clear.top,
				message->clear.left,
				message->clear.bottom,
				message->clear.right,
			        message->clear.charattr);

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
#if DEBUG_CONSOLE
		co_debug("CO_OPERATION_CONSOLE_INVERT_REGION");
#endif
		// op_invert - dummy function
		return op_invert(message->invert.y,
				 message->invert.x,
				 message->invert.count);
		break;
	default:
		co_debug("CO_OPERATION_CONSOLE_%d", message->type);
	}

	return CO_RC(ERROR);
}
