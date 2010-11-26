/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
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
#include <FL/Fl_Scroll.H>

extern "C" {
#include <colinux/user/debug.h>
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/user/reactor.h>
#include <colinux/common/console.h>
#include <colinux/os/user/daemon.h>
}

#include "widget.h"
#include "fontselect.h"

typedef struct co_console_start_parameters {
	co_id_t attach_id;
} co_console_start_parameters_t;


typedef enum {
	CO_CONSOLE_STATE_DETACHED,
	CO_CONSOLE_STATE_ATTACHED,
} co_console_state_t;

class console_log_window;
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
    /**
     * Thread message identifiers for asynch messages.
     */
    enum tm_id
    {
        TMSG_LOG_WINDOW, TMSG_MONITOR_SELECT, TMSG_OPTIONS,
        TMSG_VIEW_RESIZE,
        TMSG_LAST
    };
    /**
     * Thread messages data structure.
     */
    struct tm_data_t
    {
        tm_id       id;     // Message ID
        unsigned    value;  // 32 bit data value
        void    *   data;   // extra data
        tm_data_t( tm_id i=tm_id(0), unsigned v=0, void* d=0 )
                    : id(i), value(v), data(d) {}
    };

	console_window_t();
	~console_window_t();

	co_rc_t parse_args(int argc, char **argv);
	co_rc_t start();
	void finish();

	co_rc_t attach();
	co_rc_t attach_anyhow(co_id_t id);
	co_rc_t about();
	co_rc_t detach();
	co_rc_t send_ctrl_alt_del(co_linux_message_power_type_t poweroff);
	void idle();
	void select_monitor();

	void handle_message(co_message_t *message);
	void handle_scancode(co_scan_code_t sc);
	console_widget_t * get_widget();
	void log(const char *format, ...);
	void resize_font(void);
	FontSelectDialog* fsd;

	int get_exitdetach(void) { return reg_exitdetach; };
	void toggle_exitdetach(void) { reg_exitdetach=! reg_exitdetach; };
        console_log_window *wLog_;

protected:
	co_console_state_t state;
	co_id_t attached_id;
	bool_t resized_on_attach;
	console_widget_t *widget;
	console_main_window_t *window;
	co_console_start_parameters_t start_parameters;
	co_reactor_t reactor;
	co_user_monitor_t *message_monitor;

        // Child widgets
	Fl_Menu_Bar *menu;
        Fl_Scroll   *wScroll_;
        Fl_Group  *wStatus_;
        Fl_Box    *status_line_;
        Fl_Button *btn_log_;

	Fl_Menu_Item *find_menu_item_by_callback(Fl_Callback *cb);
	void menu_item_activate(Fl_Callback *cb);
	void menu_item_deactivate(Fl_Callback *cb);
	void global_resize_constraint();

	static co_rc_t message_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size);

	// registry settings
	int reg_font, reg_font_size, reg_copyspaces, reg_exitdetach;

};

extern void console_idle(void *data);

#endif
