#ifndef __COLINUX_WINNT_USER_CONET_DAEMON_DAEMON_H__
#define __COLINUX_WINNT_USER_CONET_DAEMON_DAEMON_H__

#include <colinux/user/daemon-base/main.h>

extern "C" {
#include <colinux/user/debug.h>
#include <colinux/os/current/user/reactor.h>
}

class user_network_tap_daemon_t : public user_daemon_t {
public:
	user_network_tap_daemon_t();
	virtual ~user_network_tap_daemon_t();
	virtual co_module_t get_base_module();
	virtual unsigned int get_unit_count();
	virtual const char *get_daemon_name();
	virtual const char *get_daemon_title();
	virtual const char *get_extended_syntax();
	virtual void received_from_monitor(co_message_t *message);
	virtual void received_from_tap(unsigned char *buffer, unsigned long size);
	virtual void handle_extended_parameters(co_command_line_params_t cmdline);
	virtual void prepare_for_loop();
	virtual void syntax();

protected:
	bool_t tap_name_specified;
	char tap_name[0x100];
	co_winnt_reactor_packet_user_t tap_handle;
};


#endif
