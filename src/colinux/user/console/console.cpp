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
#include "about.h"
#include "select_monitor.h"

#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Button.H>

extern "C" {
#include <colinux/common/messages.h>
#include <colinux/user/monitor.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/alloc.h>
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
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->send_ctrl_alt_del(0);
}

static void console_send_power_off_cb(Fl_Widget *widget, void* v)
{
	((console_window_t *)(((Fl_Menu_Item *)v)->user_data_))->send_ctrl_alt_del(1);
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
	long last_focus = keyboard_focus;

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
	co_rc_t rc;

	/* Default settings */
	start_parameters.attach_id = CO_INVALID_ID;
	attached_id = CO_INVALID_ID;
	state =	CO_CONSOLE_STATE_DETACHED;
	window = 0;
	widget = 0;
	resized_on_attach = PTRUE;
	rc = co_reactor_create(&reactor);
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

		if (strcmp(*param_scan, (option = "-a")) == 0) {
			param_scan++;
			argc--;

			if (argc <= 0) {
				co_terminal_print(
					"Parameter of command line option %s not specified\n",
					option);
				return CO_RC(ERROR);
			}

			start_parameters.attach_id = atoi(*param_scan);
		} else
		if (strcmp(*param_scan, (option = "-p")) == 0) {
			co_rc_t rc;

			param_scan++;
			argc--;

			if (argc <= 0) {
				co_terminal_print(
					"Parameter of command line option %s not specified\n",
					option);
				return CO_RC(ERROR);
			}

			rc = read_pid_from_file(*param_scan, &start_parameters.attach_id);
			if (!CO_OK(rc)) {
				co_terminal_print(
					"error on reading PID from file '%s'\n", *param_scan);
				return CO_RC(ERROR);
			}
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
#if 0
		{ "Pause", 0, (Fl_Callback *)console_pause_cb, this,  },
		{ "Resume", 0, (Fl_Callback *)console_resume_cb, this, },
#endif
		{ "Terminate", 0, (Fl_Callback *)console_terminate_cb, this, },
		{ "Reboot - send Ctrl-Alt-Del", 0, (Fl_Callback *)console_send_ctrl_alt_del_cb, this, },
		{ "Shutdown", 0, (Fl_Callback *)console_send_power_off_cb, this, },
		{ 0 },

#if 0
		{ "Inspect", 0, 0, 0, FL_SUBMENU },
		{ 0 },
#endif

		{ "Help", 0, 0, 0, FL_SUBMENU },
		{ "About", 0, (Fl_Callback *)console_about_cb, this, },
		{ 0 },

		{ 0 }
	};

	unsigned int i;
	for (i=0; i < sizeof(console_menuitems)/sizeof(console_menuitems[0]); i++)
		console_menuitems[i].user_data((void *)this);

	int swidth = 640;
	int sheight = 480;

	menu = new Fl_Menu_Bar(0, 0, swidth, 30);
	menu->box(FL_UP_BOX);
	menu->align(FL_ALIGN_CENTER);
	menu->when(FL_WHEN_RELEASE_ALWAYS);
	menu->copy(console_menuitems, window);

	Fl_Group *tile = new Fl_Group(0, 30, swidth, sheight-30);
	widget = new console_widget_t(0, 30, swidth, sheight-120);
	text_widget = new Fl_Text_Display(0, sheight-120+30, swidth, 70);

	Fl_Group *tile2 = new Fl_Group(0, sheight-120+30, swidth, 90);
	text_widget->buffer(new Fl_Text_Buffer());
	text_widget->insert_position(0);

	Fl_Box *box = new Fl_Box(0, sheight-20, swidth, 20);
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

	// Default Font is "Terminal" with size 18
	// Sample WinNT environment: set COLINUX_CONSOLE_FONT=Lucida Console:12
	// Change only font size:    set COLINUX_CONSOLE_FONT=:12
	char * env_font = getenv ("COLINUX_CONSOLE_FONT");
	if (env_font) {
		char *p = strchr (env_font, ':');

		if (p) {
			int size = atoi (p+1);
			if (size >= 4 && size <= 24) {
				// Set size
				widget->set_font_size(size);
			}
			*p = 0; // End for Fontname
		}
		
		// Set new font style
		if (strlen (env_font)) {
			// Remember: set_font need a non stack buffer!
			// Environment is global static.
			Fl::set_font(FL_SCREEN, env_font);

			// Now check font width
			fl_font(FL_SCREEN, 18); // Use default High for test here
			if ((int)fl_width('i') != (int)fl_width('W')) {
				Fl::set_font(FL_SCREEN, "Terminal"); // Restore standard font
				log("%s: is not fixed font. Using 'Terminal'\n", env_font);
			}
		}
	}

	log("Cooperative Linux console started\n");
	
	if (start_parameters.attach_id != CO_INVALID_ID)
		attached_id = start_parameters.attach_id;

	if (attached_id == CO_INVALID_ID)
		attached_id = find_first_monitor();

	if (attached_id != CO_INVALID_ID)
		attach(); /* Ignore errors, as we can attach latter */

	return CO_RC(OK);
}

console_window_t *g_console;

co_rc_t console_window_t::message_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	co_message_t *message;
	unsigned long message_size;
	long size_left = size;
	long position = 0;

	while (size_left > 0) {
		message = (typeof(message))(&buffer[position]);
		message_size = message->size + sizeof(*message);
		size_left -= message_size;
		if (size_left >= 0) {
			g_console->handle_message(message);
		}
		position += message_size;
	}

	return CO_RC(OK);
}

co_rc_t console_window_t::attach()
{
	co_rc_t rc = CO_RC(OK);  
	co_module_t modules[] = {CO_MODULE_CONSOLE, };
	co_monitor_ioctl_get_console_t get_console;
	co_console_t *console;

	if (state != CO_CONSOLE_STATE_DETACHED) {
		rc = CO_RC(ERROR);
		goto out;
	}

	g_console = this;

	rc = co_user_monitor_open(reactor, message_receive,
				  attached_id, modules, 
				  sizeof(modules)/sizeof(co_module_t),
				  &message_monitor);
	if (!CO_OK(rc)) {
		log("Monitor%d: Error connecting\n", attached_id);
		return rc;
	}

	rc = co_user_monitor_get_console(message_monitor, &get_console);
	if (!CO_OK(rc)) {
		log("Monitor%d: Error getting console\n");
		return rc;
	}

	rc = co_console_create(get_console.x, get_console.y, 0, &console);
	if (!CO_OK(rc))
		return rc;

	widget->set_console(console);

	Fl::add_idle(console_idle, this);

	resized_on_attach = PFALSE;

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
	if (state != CO_CONSOLE_STATE_ATTACHED)
		return CO_RC(ERROR);

        menu_item_activate(console_select_cb);
        menu_item_deactivate(console_pause_cb);
        menu_item_deactivate(console_resume_cb);
        menu_item_deactivate(console_terminate_cb);
        menu_item_deactivate(console_detach_cb);
        menu_item_activate(console_attach_cb);	

	co_user_monitor_close(message_monitor);	

	Fl::remove_idle(console_idle, this);

	state = CO_CONSOLE_STATE_DETACHED;

	widget->redraw();

	log("Monitor%d: Detached\n", attached_id);

	return CO_RC(OK);
}

co_rc_t console_window_t::terminate()
{
	if (state != CO_CONSOLE_STATE_ATTACHED)
		return CO_RC(ERROR);

	return detach();
}

co_rc_t console_window_t::send_ctrl_alt_del(int poweroff)
{
	if (state != CO_CONSOLE_STATE_ATTACHED)
		return CO_RC(ERROR);

	struct {
		co_message_t message;
		co_linux_message_t linux_msg;
		co_linux_message_power_t data;
	} message;
	
	message.message.from = CO_MODULE_DAEMON;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_IMPORTANT;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message.linux_msg) + sizeof(message.data);
	message.linux_msg.device = CO_DEVICE_POWER;
	message.linux_msg.unit = 0;
	message.linux_msg.size = sizeof(message.data);
	
	if (poweroff == 0)
		message.data.type = CO_LINUX_MESSAGE_POWER_ALT_CTRL_DEL;
	else
		message.data.type = CO_LINUX_MESSAGE_POWER_SHUTDOWN;

	co_user_monitor_message_send(message_monitor, &message.message);

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

void console_window_t::global_resize_constraint()
{
	if (window  &&  widget) {
		if (widget->fit_x  &&  widget->fit_y) {
			int w_diff = (int)(widget->w() - widget->fit_x);
			int h_diff = (int)(widget->h() - widget->fit_y);
			
			if (h_diff != 0   ||  w_diff != 0) {
				if (resized_on_attach == PFALSE) {
					/* co_debug("%d, %d", window->w() - w_diff, window->h() - h_diff); */
					window->size(window->w() - w_diff, window->h() - h_diff);
					resized_on_attach = PTRUE;
				}
			}
		}
	}
}

void console_window_t::idle()
{
	co_rc_t rc;

	global_resize_constraint();
	rc = co_reactor_select(reactor, 1);

	if (!CO_OK(rc)) {
		rc = detach();

		// Option in environment: "COLINUX_CONSOLE_EXIT_ON_DETACH=1"
		// Exit Console after detach
		if (CO_OK(rc) && state != CO_CONSOLE_STATE_ATTACHED) {
			char * str = getenv ("COLINUX_CONSOLE_EXIT_ON_DETACH");
			if (str && atoi(str) != 0) {
				log("Console exit after detach\n");
				exit (0);
			}
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
	// Show the "About Box" dialog
	about_box * win = new about_box( );
	win->set_modal( );
	win->show( );

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

	default: {
		if (message->type == CO_MESSAGE_TYPE_STRING) {
			co_module_name_t module_name;

			((char *)message->data)[message->size - 1] = '\0';
			log("%s: %s", co_module_repr(message->from, &module_name), message->data);
		}
		break;
	}
	}
}

// nlucas: this code is identical to console_window_t::handle_scancode
//         in colinux\user\console-base\console.cpp.
void console_window_t::handle_scancode(co_scan_code_t sc)
{
	if (state != CO_CONSOLE_STATE_ATTACHED)
		return;
		
	struct {
		co_message_t		message;
		co_linux_message_t	msg_linux;
		co_scan_code_t		code;
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

	co_user_monitor_message_send(message_monitor, &message.message);
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

