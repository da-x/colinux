/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_USER_SELECT_MONITOR_H__
#define __COLINUX_USER_SELECT_MONITOR_H__

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Button.H>

extern "C" {
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/common/console.h>
}

class select_monitor_widget_t : public Fl_Double_Window {
public:
	select_monitor_widget_t(int w, int h, const char *label = 0)
		: Fl_Double_Window(w, h, label), id_map_count(0) {};
	select_monitor_widget_t(int x, int y, int w, int h, const char *label = 0)
		: Fl_Double_Window(x, y, w, h, label), id_map_count(0) {};

	void on_button(Fl_Widget *widget);
	void populate(class console_window_t *console_window);
	void load_monitors_list();

protected:
	Fl_Button		  *	select_button;
	Fl_Button		  *	cancel_button;
	Fl_Hold_Browser   *	browser;
	co_id_t			  	id_map[CO_MAX_MONITORS];
	int					id_map_count;
	console_window_t  *	console;
};

#endif
