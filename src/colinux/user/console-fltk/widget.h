/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __COLINUX_USER_CONSOLE_WIDGET_H__
#define __COLINUX_USER_CONSOLE_WIDGET_H__

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>

extern "C" {
#include <colinux/common/console.h>
}

class console_widget_t : public Fl_Widget {
public:
	console_widget_t(int x, int y, int w, int h, const char* label=0); 
	void set_font_size(int size);
	void set_console(co_console_t *_console);
	co_console_t *get_console();
	void damage_console(int x, int y, int w, int h);
	co_rc_t handle_console_event(co_console_message_t *message);

	int fit_x;
	int fit_y;

protected:
	int font_size;
	co_console_t *console;
	int letter_x;
	int letter_y;
	double cursor_blink_interval;
	int cursor_blink_state;

	virtual void draw();
	static void static_blink_handler(class console_widget_t *widget);
	void blink_handler();
};

#endif
