/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __COLINUX_USER_CONSOLE_H__
#define __COLINUX_USER_CONSOLE_H__

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Menu.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Widget.H>

extern "C" {
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/common/console.h>
#include <colinux/common/keyboard.h>
#include <colinux/os/user/poll.h>
}

#include "widget.h"

typedef struct co_console_start_parameters {
	co_id_t attach_id;
} co_console_start_parameters_t;


typedef enum {
	CO_CONSOLE_STATE_DETACHED,
	CO_CONSOLE_STATE_ATTACHED,
} co_console_state_t;

class console_window_t;

class console_main_window_t : public Fl_Double_Window 
{
public:
	console_main_window_t(console_window_t *console);

	int keyboard_focus;

protected:
	console_window_t *console;

	virtual int handle(int event);
};

class console_window_t 
{
public:
	console_window_t();
	~console_window_t();

	co_rc_t parse_args(int argc, char **argv);
	co_rc_t start();
	void finish();
	
	co_rc_t attach();
	co_rc_t attach_anyhow(co_id_t id);
	co_rc_t pause();
	co_rc_t about();
	co_rc_t resume();
	co_rc_t detach();
	co_rc_t terminate();
	void idle();
	void select_monitor();
	void poll_callback(co_os_poll_t poll);

	void handle_message(co_console_message_t *message);
	void handle_scancode(co_scan_code_t sc);

	void log(const char *format, ...);

protected:
	co_console_state_t state;
	co_id_t attached_id;
	console_widget_t *widget;
	console_main_window_t *window;
	co_console_start_parameters_t start_parameters;
	co_os_poll_t poll;
	co_os_poll_chain_t poll_chain;
	co_user_monitor_t *monitor_handle;
	unsigned char poll_buffer[0x1000];

	Fl_Menu_Bar *menu;
	Fl_Text_Display *text_widget;

	Fl_Menu_Item *find_menu_item_by_callback(Fl_Callback *cb);
	void menu_item_activate(Fl_Callback *cb);
	void menu_item_deactivate(Fl_Callback *cb);
};

extern void console_idle(void *data);

#endif
