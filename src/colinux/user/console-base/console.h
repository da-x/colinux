/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
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
#include <colinux/os/user/daemon.h>
#include "daemon.h"
}

#include "widget.h"

typedef struct co_console_start_parameters {
	co_id_t attach_id;
} co_console_start_parameters_t;

typedef enum {
	CO_CONSOLE_STATE_OFFLINE,
	CO_CONSOLE_STATE_ONLINE,
	CO_CONSOLE_STATE_DETACHED,
	CO_CONSOLE_STATE_ATTACHED,
} co_console_state_t;

class console_window_t {
      public:
	console_window_t();
	~console_window_t();

	co_rc_t parse_args(int argc, char **argv);
	co_rc_t start();

	co_rc_t attach();
	co_rc_t attach_anyhow(co_id_t id);
	co_rc_t detach();

	void event(co_message_t & message);

	void handle_scancode(co_scan_code_t sc);

	void log(const char *format, ...);

	co_rc_t loop();

	bool online() {
		return state != CO_CONSOLE_STATE_OFFLINE;
	}
	co_rc_t online(bool);

      protected:
	co_console_start_parameters_t start_parameters;
	co_console_state_t state;
	console_widget_t *widget;
	co_id_t attached_id;
	co_daemon_handle_t daemon_handle;

	co_rc_t attached();
      public:
	co_daemon_handle_t daemonHandle() {
		return daemon_handle;
	}
};

#endif
