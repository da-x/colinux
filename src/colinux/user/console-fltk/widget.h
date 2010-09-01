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

#define MENU_SIZE_PIXELS ((int)30)

extern "C" {
#include <colinux/common/console.h>
}

class console_widget_t : public Fl_Widget {
public:

	int fit_x;
	int fit_y;
public:
	console_widget_t(int x, int y, int w, int h, const char* label = 0);
	void set_font_size(int new_size);
	void set_font_name(int new_name);
	int get_font_name(void) { return font_name; };
	int get_font_size(void) { return font_size; };
	void set_console(co_console_t* _console);
	co_console_t* get_console();
	void damage_console(int x, int y, int w, int h);
	co_rc_t handle_console_event(co_console_message_t* message);
	int screen_size_bytes(void);

	/* marking for copy to clipboard */
	void mouse_push(int x, int y, bool drag_type);
	void mouse_drag(int x, int y);
	void mouse_release(int x, int y);
	void copy_mouse_selection(char*str);
	void scroll_back_buffer(int delta);
	void scroll_page_up(void);
	void scroll_page_down(void);
	void toggle_copy_spaces(void) { copy_spaces = !copy_spaces; }
	bool get_copy_spaces(void) { return copy_spaces; }

protected:
	int           font_size, font_name;
	co_console_t* console;
	co_console_cell_t* cell_limit;
	int 	      letter_x;
	int 	      letter_y;
	double	      cursor_blink_interval;
	int 	      cursor_blink_state;
	int           cursize_tab[CO_CUR_BLOCK+1];

	bool 		mouse_copy, mouse_drag_type, copy_spaces;
	int 		mouse_start_x, mouse_start_y, mouse_sx, mouse_wx,
		mouse_sy, mouse_wy, loc_start, loc_end;

	int scroll_lines;

protected:
	virtual void draw();
	static void static_blink_handler(class console_widget_t* widget);
	void blink_handler();
	void invert_area(int sx, int sy, int wx, int wy);
	void calc_area(int x, int y);
	int loc_x(int mouse_x);
	int loc_y(int mouse_y);
	void mouse_clear(void);
	void update_font(void);

	/* internal helper functions */
	int i_min(int a, int b) { return (a<b) ? a : b; };
	int i_max(int a, int b) { return (a>b) ? a : b; };
	int i_abs(int a) { return (a>0) ? a : -a; };

};

#endif
