/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "console.h"
#include "select_monitor.h"

#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Button.H>

extern "C" {
#include <colinux/user/monitor.h>
#include <colinux/os/user/misc.h>
}

static void console_window_cb(Fl_Widget *widget, void* v) 
{
	((console_window_t *)v)->finish();
}

static void console_quit_cb(Fl_Widget *widget, void* v) 
{
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->finish();
}

static void console_select_cb(Fl_Widget *widget, void* v) 
{
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->select_monitor();
}

static void console_attach_cb(Fl_Widget *widget, void* v) 
{
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->attach();
}

static void console_detach_cb(Fl_Widget *widget, void* v) 
{
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->detach();
}

static void console_pause_cb(Fl_Widget *widget, void* v) 
{
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->pause();
}

static void console_resume_cb(Fl_Widget *widget, void* v) 
{
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->resume();
}

static void console_terminate_cb(Fl_Widget *widget, void* v) 
{
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->terminate();
}

static void console_poll_callback(co_os_poll_t poll, void *data)
{
	((console_window_t *)data)->poll_callback(poll);
}

static void console_about_cb(Fl_Widget *widget, void* v) 
{
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->about();
}

void console_idle(void *data)
{
	((console_window_t *)data)->idle();
}

console_main_window_t::console_main_window_t(console_window_t *console)
	: Fl_Double_Window(640, 480), console(console)
{
	label("Cooperative Linux console");
}

int console_main_window_t::handle(int event)
{
	switch (event) {
	case FL_FOCUS:
		keyboard_focus = 1;
		return 1;

	case FL_UNFOCUS:
		keyboard_focus = 0;
		break;
	}

	return Fl_Double_Window::handle(event);
}

console_window_t::console_window_t()
{
	/* Default settings */
	start_parameters.attach_id = CO_INVALID_ID;	
	attached_id = CO_INVALID_ID;
	state =	CO_CONSOLE_STATE_DETACHED;
	poll_chain = 0;
	poll = 0;
	window = 0;
	widget = 0;
	monitor_handle = 0;
}

console_window_t::~console_window_t()
{
}

co_rc_t console_window_t::parse_args(int argc, char **argv)
{
	char **param_scan = argv;

	/* Parse command line */
	while (argc > 0) {
		const char *option;

		option = "-a";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			argc--;

			if (argc <= 0) {
				co_terminal_print(
					"Parameter of command line option %s not specified\n",
					option);
				return CO_RC(ERROR);
			}

			start_parameters.attach_id = atoi(*param_scan);
		}

		param_scan++;
		argc--;
	}

	return CO_RC(OK);
}

co_rc_t console_window_t::start()
{
	co_rc_t rc;

	rc = co_os_poll_chain_create(&poll_chain);
	if (!CO_OK(rc))
		return rc;

	window = new console_main_window_t(this);
	window->callback(console_window_cb, this);

	Fl_Menu_Item console_menuitems[] = {
		{ "File", 0, 0, 0, FL_SUBMENU },
		{ "Quit", 0, (Fl_Callback *)console_quit_cb, this },
		{ 0 },

		{ "Monitor", 0, 0, 0, FL_SUBMENU },
		{ "Select", 0, (Fl_Callback *)console_select_cb, this, FL_MENU_DIVIDER },
		{ "Attach", 0, (Fl_Callback *)console_attach_cb, this, },
		{ "Detach", 0, (Fl_Callback *)console_detach_cb, this, FL_MENU_DIVIDER },
		{ "Pause", 0, (Fl_Callback *)console_pause_cb, this,  },
		{ "Resume", 0, (Fl_Callback *)console_resume_cb, this, },
		{ "Terminate", 0, (Fl_Callback *)console_terminate_cb, this, },
		{ 0 },

		{ "Inspect", 0, 0, 0, FL_SUBMENU },
		{ 0 },

		{ "Help", 0, 0, 0, FL_SUBMENU },
		{ "About", 0, (Fl_Callback *)console_about_cb, this, },
		{ 0 },

		{ 0 }
	};

	unsigned int i;
	for (i=0; i < sizeof(console_menuitems)/sizeof(console_menuitems[0]); i++)
		console_menuitems[i].user_data((void *)this);

	menu = new Fl_Menu_Bar(0, 0, 640, 30);
	menu->box(FL_UP_BOX);
	menu->align(FL_ALIGN_CENTER);
	menu->when(FL_WHEN_RELEASE_ALWAYS);
	menu->copy(console_menuitems, window);

	Fl_Group *tile = new Fl_Group(0, 30, 640, 480-30);
	widget = new console_widget_t(0, 30, 640, 480-120);
	text_widget = new Fl_Text_Display(0, 480-120+30, 640, 70);

	Fl_Group *tile2 = new Fl_Group(0, 480-120+30, 640, 90);
	text_widget->buffer(new Fl_Text_Buffer());
	text_widget->insert_position(0);

	Fl_Box *box = new Fl_Box(0, 480-20, 640, 20);
	box->label("");
	box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE); 
	tile2->end();

	tile->resizable(widget);
	tile->end();

	window->resizable(tile); 
	window->end();
	window->show();

	menu_item_activate(console_select_cb);
	menu_item_deactivate(console_pause_cb);
	menu_item_deactivate(console_resume_cb);
	menu_item_deactivate(console_terminate_cb);
	menu_item_deactivate(console_detach_cb);
	menu_item_deactivate(console_attach_cb);

	log("Coopeartive Linux console started\n");
	
	if (start_parameters.attach_id != CO_INVALID_ID)
		attached_id = start_parameters.attach_id;

	if (attached_id != CO_INVALID_ID) 
		return attach();
	
	return CO_RC(OK);
}

co_rc_t console_window_t::attach()
{
	co_console_t *console;
	co_monitor_ioctl_status_t status;
	co_rc_t rc; 

	if (state != CO_CONSOLE_STATE_DETACHED) {
		rc = CO_RC(ERROR);
		goto out;
	}

	rc = co_user_monitor_open(attached_id, &monitor_handle);
	if (!CO_OK(rc)) {
		log("Error opening COLX driver\n");
		rc = CO_RC(ERROR);
		goto out;
	}

	rc = co_user_monitor_status(monitor_handle, &status);
	if (!CO_OK(rc)) {
		log("Error attaching to Monitor%d\n", attached_id);
		return rc;
	}

	rc = co_user_monitor_get_console(monitor_handle, &console);
	if (!CO_OK(rc)) {
		log("Error getting console (%d)\n", rc);
		goto out_close;
	}

	widget->set_console(console);

	state = CO_CONSOLE_STATE_ATTACHED;

	rc = co_os_poll_create(attached_id,
			       CO_MONITOR_IOCTL_CONSOLE_POLL,
			       CO_MONITOR_IOCTL_CONSOLE_CANCEL_POLL,
			       console_poll_callback,
			       this,
			       poll_chain,
			       &poll);
	
	/* Todo: handle error... */

	menu_item_deactivate(console_select_cb);
	menu_item_activate(console_pause_cb);
	menu_item_deactivate(console_resume_cb);
	menu_item_activate(console_terminate_cb);
	menu_item_activate(console_detach_cb);
	menu_item_deactivate(console_attach_cb);
	
	widget->redraw();

	log("Monitor%d: Attached\n", attached_id);
	
	return rc;

out_close:
	co_user_monitor_close(monitor_handle);

out:
	return rc;
}

co_rc_t console_window_t::pause()
{
	log("Pause not implemented yet");
	return CO_RC(OK);
}

co_rc_t console_window_t::resume()
{
	log("Pause not implemented yet");
	return CO_RC(OK);
}

co_rc_t console_window_t::attach_anyhow(co_id_t id)
{
	co_rc_t rc;

	if (state == CO_CONSOLE_STATE_ATTACHED) {
		rc = detach();
		if (!CO_OK(rc))
			return rc;
	}

	attached_id = id;
	return attach();
}

co_rc_t console_window_t::detach()
{
	co_console_t *console;

	if (state != CO_CONSOLE_STATE_ATTACHED)
		return CO_RC(ERROR);

	co_os_poll_destroy(poll);	

	console = widget->get_console();
	co_user_monitor_put_console(monitor_handle, console);

	co_user_monitor_close(monitor_handle);	
	monitor_handle = NULL;

	menu_item_activate(console_select_cb);
	menu_item_deactivate(console_pause_cb);
	menu_item_deactivate(console_resume_cb);
	menu_item_deactivate(console_terminate_cb);
	menu_item_deactivate(console_detach_cb);
	menu_item_activate(console_attach_cb);

	state = CO_CONSOLE_STATE_DETACHED;

	widget->redraw();

	log("Monitor%d: Detached\n", attached_id);

	return CO_RC(OK);
}

co_rc_t console_window_t::terminate()
{
	co_rc_t rc;
	if (state != CO_CONSOLE_STATE_ATTACHED)
		return CO_RC(ERROR);
	
	rc = co_user_monitor_terminate(monitor_handle);
	if (!CO_OK(rc)) {
		co_debug("Error terminating: %d\n", rc);
		return rc;
	}
	
	return detach();
}

void console_window_t::finish()
{
	if (state == CO_CONSOLE_STATE_ATTACHED)
		detach();

	if (poll_chain)
		co_os_poll_chain_destroy(poll_chain);

	exit(0);
}

void console_window_t::idle()
{
	co_os_poll_chain_wait(poll_chain);
}

void console_window_t::select_monitor()
{
	select_monitor_widget_t *dialog = new select_monitor_widget_t(400, 220);
	dialog->populate(this);
}

co_rc_t console_window_t::about()
{
	Fl_Double_Window *win = new Fl_Double_Window(400, 300);
	win->end();
}

void console_window_t::handle_message(co_monitor_ioctl_console_message_t *message)
{
	switch (message->type) {
	case CO_MONITOR_IOCTL_CONSOLE_MESSAGE_NORMAL: {
		co_console_message_t *console_message;

		console_message = (typeof(console_message))(message->extra_data); 
		
		widget->handle_console_event(console_message);

		break;
	}
	case CO_MONITOR_IOCTL_CONSOLE_MESSAGE_TERMINATED:
		break;
	}
}

void console_window_t::handle_scancode(co_scan_code_t sc)
{
	if (state != CO_CONSOLE_STATE_ATTACHED)
		return;
		
	if (!window || !window->keyboard_focus)
		return;

	co_monitor_ioctl_keyboard_t params;
	params.op = CO_OPERATION_KEYBOARD_ACTION;
	params.sc = sc;

	co_user_monitor_keyboard(monitor_handle, &params);
}

void console_window_t::poll_callback(co_os_poll_t poll)
{
	if (state == CO_CONSOLE_STATE_ATTACHED) {
		co_monitor_ioctl_console_messages_t *params;
		co_rc_t rc;

		params = (typeof(params))(&poll_buffer);

		rc = co_user_monitor_console_messages(monitor_handle, 
						      params, sizeof(poll_buffer));

		if (!CO_OK(rc)) 
			co_debug("poll failed\n");
		else {
			int i=0;
			char *param_data = params->data;
			co_monitor_ioctl_console_message_t *message;

			for (i=0; i < params->num_messages; i++) {
				message = (typeof(message))param_data;
				
				handle_message(message);

				param_data += message->size;
			}
		}
	}
}

Fl_Menu_Item *console_window_t::find_menu_item_by_callback(Fl_Callback *cb)
{
	return NULL;
}

void console_window_t::menu_item_activate(Fl_Callback *cb)
{
	int i;
	int count = menu->size();
	const Fl_Menu_Item *items = menu->menu();
	Fl_Menu_Item items_copy[count];

	for (i=0; i < count; i++) {
		items_copy[i] = items[i];
		if (items_copy[i].callback() == cb)
			items_copy[i].activate();
	}

	menu->copy(items_copy);
}

void console_window_t::menu_item_deactivate(Fl_Callback *cb)
{
	int i;
	int count = menu->size();
	const Fl_Menu_Item *items = menu->menu();
	Fl_Menu_Item items_copy[count];

	for (i=0; i < count; i++) {
		items_copy[i] = items[i];
		if (items_copy[i].callback() == cb)
			items_copy[i].deactivate();
	}

	menu->copy(items_copy);
}

void console_window_t::log(const char *format, ...)
{
	char buf[0x100];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	text_widget->insert_position(text_widget->buffer()->length());
	text_widget->show_insert_position();
	text_widget->insert(buf);	
}

