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

#ifndef __COLINUX_USER_DAEMON_BASE_H__
#define __COLINUX_USER_DAEMON_BASE_H__

extern "C" {
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/user/reactor.h>
#include <colinux/user/cmdline.h>
#include <colinux/os/alloc.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/user/daemon.h>
#include <colinux/common/console.h>
#include <colinux/common/libc.h>
}

class user_daemon_exception_t {
public:
	co_rc_t rc;

	user_daemon_exception_t(co_rc_t _rc) : rc(_rc) {};
};

class user_daemon_t {
public:
	user_daemon_t();
	virtual ~user_daemon_t();

	virtual void run(int argc, char *argv[]);
	virtual co_module_t get_base_module()=0;
	virtual unsigned int get_unit_count()=0;
	virtual const char *get_daemon_name()=0;
	virtual const char *get_daemon_title()=0;
	virtual const char *get_extended_syntax();
	virtual void log(const char *format, ...);
	virtual void received_from_monitor(co_message_t *message)=0;
	virtual void send_to_monitor(co_message_t *message);
	virtual void handle_parameters(int argc, char *argv[]);
	virtual void handle_extended_parameters(co_command_line_params_t cmdline);
	virtual void verify_parameters();
	virtual void syntax();
	virtual void prepare_for_loop();
	virtual void send_to_monitor_raw(co_device_t device, unsigned char *buffer, unsigned long size);

protected:
	co_reactor_t reactor;
	co_user_monitor_t *monitor_handle;
	unsigned int param_index;
	co_id_t param_instance;

};

#endif
