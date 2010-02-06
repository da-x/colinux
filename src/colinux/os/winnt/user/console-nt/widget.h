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
#include <colinux/user/console-nt/console.h>
}

class console_widget_NT_t:public console_widget_t {
public:
	console_widget_NT_t();
	~console_widget_NT_t();

	void draw();
	co_rc_t loop();
	co_rc_t set_window(console_window_t* w);
	co_rc_t console_window(class console_window_t*);
	co_rc_t idle();
	co_rc_t title(const char*);
	void update();
	co_rc_t set_cursor_size(const int cursor_size);

protected:
	HANDLE 		    own_out_h; // Handle of the own screen buffer
	HANDLE		    std_out_h; // Handle of the standard screen buffer
	HANDLE		    input;

	CHAR_INFO 	    blank;
	SMALL_RECT 	    win_region;
	COORD 		    win_size;
	COORD 		    buf_size;
	CHAR_INFO*	    screen;
	CONSOLE_CURSOR_INFO cursor_info;

	HWND		    saved_hwnd;
        DWORD		    saved_mode;
	HANDLE		    saved_output;
        HANDLE		    saved_input;
	CONSOLE_CURSOR_INFO saved_cursor;
        bool		    allocated_console;

	void save_standard_console();
	void restore_console();

	void process_key_event(KEY_EVENT_RECORD& ker);
	void process_focus_event(FOCUS_EVENT_RECORD& fer);

	co_rc_t op_scroll_up(
		const co_console_unit &topRow,
		const co_console_unit &bottomRow,
		const co_console_unit &lines,
		const co_console_character &charattr);

	co_rc_t op_scroll_down(
		const co_console_unit &topRow,
		const co_console_unit &bottomRow,
		const co_console_unit &lines,
		const co_console_character &charattr);

	co_rc_t op_putcs(
		const co_console_unit &Y,
		const co_console_unit &X,
		const co_console_character *data,
		const co_console_unit &length);

	co_rc_t op_putc(
		const co_console_unit &Y,
		const co_console_unit &X,
		const co_console_character& charattr);

	co_rc_t op_cursor_pos(
		const co_cursor_pos_t& position);

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
