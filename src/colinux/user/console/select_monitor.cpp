/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdio.h>

#include "select_monitor.h"
#include "console.h"

static void console_select_monitor(Fl_Widget *widget, select_monitor_widget_t *window)
{
	window->on_button(widget);
}

void select_monitor_widget_t::on_button(Fl_Widget *widget)
{
	if (widget == select_button) {
		int selection = browser->value();
		if (selection != 0) {
			co_id_t id = id_map[selection-1];
			console->attach_anyhow(id);
			delete this;
		}
	} else if (widget == cancel_button) {
		delete this;
	}
}

void select_monitor_widget_t::populate(console_window_t *console_window)
{
	label("Select a running coLinux");

	console = console_window;

	browser = new Fl_Hold_Browser(0, 0, 400, 200);
	Fl_Button *button;

	button = new Fl_Button(0, 200, 200, 20);
	button->label("Select");
	button->when(FL_WHEN_RELEASE);
	button->callback((Fl_Callback *)console_select_monitor, this);
	select_button = button;

	button = new Fl_Button(200, 200, 200, 20);
	button->label("Cancel");
	button->when(FL_WHEN_RELEASE);
	button->callback((Fl_Callback *)console_select_monitor, this);
	cancel_button = button;

	resizable(browser);

	end();
	show();

	load_monitors_list();
}

void select_monitor_widget_t::load_monitors_list()
{
	int i = 0;

	if (id_map)
		delete []id_map;

	id_map = new co_id_t[CO_MAX_MONITORS];
	id_map_size = 0;

	browser->clear();

	char buf[0x100];
	
	snprintf(buf, sizeof(buf), "Monitor%d\t", i);
	
	id_map[id_map_size] = 0;
	id_map_size++;
}
