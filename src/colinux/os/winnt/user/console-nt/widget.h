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
	co_rc_t console_window(class console_window_t *);
	co_rc_t idle();
	co_rc_t title(const char *);
	void co_console_update();

      protected:
	HANDLE buffer;
	CHAR_INFO blank;
	SMALL_RECT region;
	COORD size;
	CHAR_INFO *screen;
	HANDLE input;
	HANDLE output;
	CONSOLE_CURSOR_INFO cursor;
	unsigned keyed;

	co_rc_t op_scroll_up(co_console_unit &topRow, co_console_unit &bottomRow, co_console_unit &lines);
	co_rc_t op_scroll_down(co_console_unit &topRow, co_console_unit &bottomRow, co_console_unit &lines);
	co_rc_t op_putcs(co_console_unit &Y, co_console_unit &X, co_console_character *data, co_console_unit &length);
	co_rc_t op_putc(co_console_unit &Y, co_console_unit &X, co_console_character &charattr);
	co_rc_t op_cursor(co_cursor_pos_t & position);
	co_rc_t op_clear(co_console_unit &T, co_console_unit &L,
			 co_console_unit &B, co_console_unit &R, co_console_character charattr);
	co_rc_t op_bmove(co_console_unit &Y, co_console_unit &X,
			 co_console_unit &T, co_console_unit &L,
			 co_console_unit &B, co_console_unit &R);
	co_rc_t op_invert(co_console_unit &Y, co_console_unit &X, co_console_unit &C);
};

#endif
