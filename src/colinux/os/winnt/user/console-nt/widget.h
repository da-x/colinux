/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
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

      protected:
	 HANDLE buffer;
	CHAR_INFO blank;
	SMALL_RECT region;
	COORD size;
	CHAR_INFO *screen;
	HANDLE input;
	HANDLE previous_output;
	CONSOLE_CURSOR_INFO previous_cursor;
	bool allocatedConsole;
	unsigned keyed;

	co_rc_t op_scroll(long &topRow, long &bottomRow, long &direction,
			  long &lines);
	co_rc_t op_putcs(long &Y, long &X, char *data, long &length);
	co_rc_t op_putc(long &Y, long &X, unsigned short &charattr);
	co_rc_t op_cursor(co_cursor_pos_t & position);

};

#endif
