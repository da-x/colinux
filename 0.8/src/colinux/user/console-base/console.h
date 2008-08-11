/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2005 (c)
 * Ballard, Jonathan H.  <californiakidd@users.sourceforge.net>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_USER_CONSOLE_H__
#define __COLINUX_USER_CONSOLE_H__

extern "C" {
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/common/console.h>
#include <colinux/user/reactor.h>
#include <colinux/os/user/daemon.h>
}

#include "widget.h"

typedef struct co_console_start_parameters {
	co_id_t attach_id;
} co_console_start_parameters_t;

class console_window_t {
public:
	console_window_t();
	virtual ~console_window_t();

	co_rc_t parse_args(int argc, char **argv);
	co_rc_t start();

	co_rc_t attach();
	co_rc_t detach();

	void event(co_message_t *message);

	void handle_scancode(const co_scan_code_t sc) const;
	co_rc_t send_ctrl_alt_del();
	void syntax();

	virtual const char *get_name();

	void log(const char *format, ...) const;
	bool_t is_attached() { return attached ; };

	co_rc_t loop();

protected:
	co_console_start_parameters_t start_parameters;
	console_widget_t *widget;
	co_id_t instance_id;
	co_reactor_t reactor;
	co_user_monitor_t *message_monitor;
	bool_t attached;

	static co_rc_t message_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size);
};

#endif
