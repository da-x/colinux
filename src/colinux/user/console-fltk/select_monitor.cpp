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
#include <string.h>

#include "select_monitor.h"
#include "console.h"

static void console_select_monitor(Fl_Widget* widget, select_monitor_widget_t* window)
{
	window->on_button(widget);
}

void select_monitor_widget_t::on_button(Fl_Widget* widget)
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

void select_monitor_widget_t::populate(console_window_t* console_window)
{
	label("Select a running coLinux");

	console = console_window;

	browser = new Fl_Hold_Browser(0, 0, 400, 200);
	Fl_Button* button;

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

	load_monitors_list();
	show();
}

void select_monitor_widget_t::load_monitors_list()
{
	co_manager_handle_t		handle;
	co_manager_ioctl_monitor_list_t	list;
	co_rc_t				rc;

	memset(id_map, 0, sizeof(id_map));
	id_map_count = 0;
	browser->clear();

	handle = co_os_manager_open();
	if (handle == NULL)
		return;

	rc = co_manager_monitor_list(handle, &list);
	co_os_manager_close(handle);
	if (!CO_OK(rc))
		return;

	for (unsigned i = 0; i < list.count; ++i) {
		char buf[32];

		id_map[i] = list.ids[i];
		snprintf(buf, sizeof(buf), "Monitor%d (pid=%d)\t", i, (int)id_map[i]);
		browser->add(buf);
	}

	id_map_count = list.count;
}
