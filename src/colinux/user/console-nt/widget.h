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

#ifndef __COLINUX_USER_CONSOLE_WIDGET_H__
#define __COLINUX_USER_CONSOLE_WIDGET_H__

extern "C" {
#include <colinux/common/console.h>
}

class console_widget_t {
public:
	console_widget_t();
	virtual ~console_widget_t() = 0;

	void set_console(co_console_t* _console);
	co_rc_t event(co_console_message_t* message);
	void redraw();

	virtual co_rc_t loop() = 0;
	virtual co_rc_t set_window(class console_window_t* win_p) = 0;
	virtual co_rc_t set_cursor_size(const int size_prc) = 0;
	virtual void update() = 0;
	virtual co_rc_t title(const char* title_str) = 0;
	virtual co_rc_t idle() = 0;

protected: // variables
	class console_window_t*	window;
	co_console_t*		console;


protected: // methods
	virtual void draw() = 0;

	virtual co_rc_t op_putcs(
		const co_console_unit&      Y,
		const co_console_unit&      X,
		const co_console_character* data,
		const co_console_unit&      length) = 0;

	virtual co_rc_t op_putc(
		const co_console_unit&      Y,
		const co_console_unit&      X,
		const co_console_character& charattr) = 0;

	virtual co_rc_t op_cursor_pos(
		const co_cursor_pos_t& position) = 0;

	virtual co_rc_t op_clear(
		const co_console_unit&     T,
		const co_console_unit&     L,
		const co_console_unit&     B,
		const co_console_unit&     R,
		const co_console_character charattr) = 0;

	// Scroll/move screen buffer contents
	virtual co_rc_t op_scroll_up(
		const co_console_unit& topRow,
		const co_console_unit& bottomRow,
		const co_console_unit& lines,
		const co_console_character& charattr) = 0;

	virtual co_rc_t op_scroll_down(
		const co_console_unit& topRow,
		const co_console_unit& bottomRow,
		const co_console_unit& lines,
		const co_console_character& charattr) = 0;

	virtual co_rc_t op_bmove(
		const co_console_unit& Y,
		const co_console_unit& X,
		const co_console_unit& T,
		const co_console_unit& L,
		const co_console_unit& B,
		const co_console_unit& R) = 0;

	// Set inverce color
	virtual co_rc_t op_invert(
		const co_console_unit& Y,
		const co_console_unit& X,
		const co_console_unit& C) = 0;
};

extern "C" console_widget_t* co_console_widget_create();

#endif
