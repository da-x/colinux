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
#include <colinux/os/alloc.h>
#include <colinux/os/user/daemon.h>
}

#include "main.h"

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

static void console_send_ctrl_alt_del_cb(Fl_Widget *widget, void* v) 
{
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->send_ctrl_alt_del();
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
	unsigned long last_focus = keyboard_focus;

	switch (event) {
	case FL_FOCUS:
		keyboard_focus = 1;
		break;

	case FL_UNFOCUS:
		keyboard_focus = 0;
		break;
	}

	if (last_focus != keyboard_focus)
		co_user_console_keyboard_focus_change(keyboard_focus);

	return Fl_Double_Window::handle(event);
}

console_window_t::console_window_t()
{
	/* Default settings */
	start_parameters.attach_id = 1; 
	attached_id = CO_INVALID_ID;
	state =	CO_CONSOLE_STATE_DETACHED;
	window = 0;
	widget = 0;
	daemon_handle = 0;
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
		{ "Send Ctrl-Alt-Del", 0, (Fl_Callback *)console_send_ctrl_alt_del_cb, this, },
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

	log("Cooperative Linux console started\n");
	
	if (start_parameters.attach_id != CO_INVALID_ID)
		attached_id = start_parameters.attach_id;

	if (attached_id != CO_INVALID_ID) 
		return attach();
	
	return CO_RC(OK);
}

co_rc_t console_window_t::attach()
{
	co_rc_t rc = CO_RC(OK);  

	if (state != CO_CONSOLE_STATE_DETACHED) {
		rc = CO_RC(ERROR);
		goto out;
	}

	rc = co_os_open_daemon_pipe(0, CO_MODULE_CONSOLE, &daemon_handle);
	if (!CO_OK(rc))
		goto out;

	Fl::add_idle(console_idle, this);

	state = CO_CONSOLE_STATE_ATTACHED;

	menu_item_deactivate(console_select_cb);
	menu_item_activate(console_pause_cb);
	menu_item_deactivate(console_resume_cb);
	menu_item_activate(console_terminate_cb);
	menu_item_activate(console_detach_cb);
	menu_item_deactivate(console_attach_cb);
	
	widget->redraw();

	log("Monitor%d: Attached\n", attached_id);

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

	console = widget->get_console();
	if (console == NULL)
		return CO_RC(ERROR);

	struct {
		co_message_t message;
		co_daemon_console_message_t console;
		char data[0];
	} *message;

	co_console_pickle(console);

	message = (typeof(message))(co_os_malloc(sizeof(*message)+console->size));
	if (message) {
		message->message.to = CO_MODULE_DAEMON;
		message->message.from = CO_MODULE_CONSOLE;
		message->message.size = sizeof(message->console) + console->size;
		message->message.type = CO_MESSAGE_TYPE_STRING;
		message->message.priority = CO_PRIORITY_IMPORTANT;
		message->console.type = CO_DAEMON_CONSOLE_MESSAGE_DETACH;
		message->console.size = console->size;
		memcpy(message->data, console, console->size);
		
		co_os_daemon_send_message(daemon_handle, &message->message);
		co_os_free(message);
	}

	co_console_unpickle(console);
	co_console_destroy(console);

        menu_item_activate(console_select_cb);
        menu_item_deactivate(console_pause_cb);
        menu_item_deactivate(console_resume_cb);
        menu_item_deactivate(console_terminate_cb);
        menu_item_deactivate(console_detach_cb);
        menu_item_activate(console_attach_cb);	

	co_os_daemon_close(daemon_handle);

	Fl::remove_idle(console_idle, this);

	daemon_handle = 0;

	state = CO_CONSOLE_STATE_DETACHED;

	widget->redraw();

	log("Monitor%d: Detached\n", attached_id);

	return CO_RC(OK);
}

co_rc_t console_window_t::terminate()
{
	if (state != CO_CONSOLE_STATE_ATTACHED)
		return CO_RC(ERROR);

	struct {
		co_message_t message;
		co_daemon_console_message_t console;
	} message;
		
	message.message.from = CO_MODULE_CONSOLE;
	message.message.to = CO_MODULE_DAEMON;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message) - sizeof(message.message);
	message.console.type = CO_DAEMON_CONSOLE_MESSAGE_TERMINATE;
	message.console.size = 0;

	co_os_daemon_send_message(daemon_handle, &message.message);
	
	return detach();
}

co_rc_t console_window_t::send_ctrl_alt_del()
{
	if (state != CO_CONSOLE_STATE_ATTACHED)
		return CO_RC(ERROR);

	struct {
		co_message_t message;
		co_daemon_console_message_t console;
	} message;
		
	message.message.from = CO_MODULE_CONSOLE;
	message.message.to = CO_MODULE_DAEMON;
	message.message.priority = CO_PRIORITY_IMPORTANT;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message) - sizeof(message.message);
	message.console.type = CO_DAEMON_CONSOLE_MESSAGE_CTRL_ALT_DEL;
	message.console.size = 0;

	co_os_daemon_send_message(daemon_handle, &message.message);

	return CO_RC(OK);
}

void console_window_t::finish()
{
	if (Fl::event_key() == FL_Escape) 
		return;

	if (state == CO_CONSOLE_STATE_ATTACHED)
		detach();

	exit(0);
}

void console_window_t::idle()
{
	if (daemon_handle != NULL) {
		co_rc_t rc = CO_RC(OK);
		co_message_t *message = NULL;

		rc = co_os_daemon_get_message(daemon_handle, &message, 10);
		if (!CO_OK(rc)) {
			if (CO_RC_GET_CODE(rc) == CO_RC_BROKEN_PIPE) {
				log("Monitor%d: Broken pipe\n", attached_id);
				detach(); 
			}
			return;
		}

		if (message) {
			handle_message(message);
			co_os_free(message);
		}
	}
}

void console_window_t::select_monitor()
{
	select_monitor_widget_t *dialog = new select_monitor_widget_t(400, 220);
	dialog->populate(this);
}

co_rc_t console_window_t::about()
{
	Fl_Double_Window *win = new Fl_Double_Window(400, 300);

	/* TODO: Add some text here :) */

	win->end();

	return CO_RC(OK);
}

void console_window_t::handle_message(co_message_t *message)
{
	switch (message->from) {
	case CO_MODULE_LINUX: {
		co_console_message_t *console_message;

		console_message = (typeof(console_message))(message->data);
		widget->handle_console_event(console_message);
		break;
	}

	case CO_MODULE_DAEMON: {
		struct {
			co_message_t message;
			co_daemon_console_message_t console;
			char data[0];
		} *console_message;

		console_message = (typeof(console_message))message;

		if (console_message->console.type == CO_DAEMON_CONSOLE_MESSAGE_ATTACH) {
			co_console_t *console;

			console = (co_console_t *)co_os_malloc(console_message->console.size);
			if (!console)
				break;

			memcpy(console, console_message->data, console_message->console.size);
			co_console_unpickle(console);

			widget->set_console(console);
			widget->redraw();
		}
		
		break;
	}
	default:
		break;
	}
}

void console_window_t::handle_scancode(co_scan_code_t sc)
{
	if (state != CO_CONSOLE_STATE_ATTACHED)
		return;
		
	if (!window)
		return;

	struct {
		co_message_t message;
		co_linux_message_t msg_linux;
		co_scan_code_t code;
	} message;
		
	message.message.from = CO_MODULE_CONSOLE;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message) - sizeof(message.message);
	message.msg_linux.device = CO_DEVICE_KEYBOARD;
	message.msg_linux.unit = 0;
	message.msg_linux.size = sizeof(message.code);
	message.code = sc;

	co_os_daemon_send_message(daemon_handle, &message.message);
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

