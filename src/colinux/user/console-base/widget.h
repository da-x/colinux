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

#ifndef __COLINUX_USER_CONSOLE_WIDGET_H__
#define __COLINUX_USER_CONSOLE_WIDGET_H__

extern "C" {
#include <colinux/common/console.h>
} class console_widget_t {
      public:
	console_widget_t();
	virtual ~ console_widget_t() = 0;

	void co_console(co_console_t * _console);
	co_console_t *co_console();
	co_rc_t event(co_console_message_t * message);
	void redraw();

	virtual co_rc_t loop() = 0;
	virtual co_rc_t console_window(class console_window_t *) = 0;
	virtual co_rc_t title(const char *) = 0;

	virtual co_rc_t idle() = 0;

      protected:
	 class console_window_t * window;
	co_console_t *console;

	virtual void draw() = 0;
	virtual co_rc_t op_scroll(long &topRow, long &bottomRow,
				  long &direction, long &lines) = 0;
	virtual co_rc_t op_putcs(long &Y, long &X, char *data, long &length) = 0;
	virtual co_rc_t op_putc(long &Y, long &X, unsigned short &charattr) = 0;
	virtual co_rc_t op_cursor(co_cursor_pos_t & position) = 0;

};

extern "C" console_widget_t * co_console_widget_create();

#endif
