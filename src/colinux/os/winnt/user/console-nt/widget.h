/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 * Ballard, Jonathan H.  <californiakidd@users.sourceforge.net>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_WINNT_OS_USER_CONSOLE_WIDGET_H__
#define __COLINUX_WINNT_OS_USER_CONSOLE_WIDGET_H__

#include <windows.h>

extern "C" {
#include <colinux/user/console-base/console.h>
}

class console_widget_NT_t:public console_widget_t {
      public:
	console_widget_NT_t();
	~console_widget_NT_t();

	void draw();
	co_rc_t loop();
	co_rc_t set_window(console_window_t *w);
	co_rc_t console_window(class console_window_t *);
	co_rc_t idle();
	co_rc_t title(const char *);
	void update();

      protected:
	HANDLE buffer;
	CHAR_INFO blank;
	SMALL_RECT region;
	COORD size;
	CHAR_INFO *screen;
	HANDLE input;
	HANDLE output;
	CONSOLE_CURSOR_INFO cursor;

	void ProcessKeyEvent( KEY_EVENT_RECORD& ker );
	void ProcessFocusEvent( FOCUS_EVENT_RECORD& fer );

	co_rc_t op_scroll_up(
			const co_console_unit &topRow,
			const co_console_unit &bottomRow,
			const co_console_unit &lines);
	co_rc_t op_scroll_down(
			const co_console_unit &topRow,
			const co_console_unit &bottomRow,
			const co_console_unit &lines);
	co_rc_t op_putcs(
			const co_console_unit &Y,
			const co_console_unit &X,
			const co_console_character *data,
			const co_console_unit &length);
	co_rc_t op_putc(
			const co_console_unit &Y,
			const co_console_unit &X,
			const co_console_character &charattr);
	co_rc_t op_cursor(
			const co_cursor_pos_t &position);
	co_rc_t op_clear(
			const co_console_unit &T,
			const co_console_unit &L,
			const co_console_unit &B,
			const co_console_unit &R,
			const co_console_character charattr);
	co_rc_t op_bmove(
			const co_console_unit &Y,
			const co_console_unit &X,
			const co_console_unit &T,
			const co_console_unit &L,
			const co_console_unit &B,
			const co_console_unit &R);
	co_rc_t op_invert(
			const co_console_unit &Y,
			const co_console_unit &X,
			const co_console_unit &C);
};

#endif
