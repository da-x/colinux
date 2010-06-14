/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_USER_MONITOR_H__
#define __COLINUX_USER_MONITOR_H__

#include <colinux/common/common.h>
#include <colinux/common/console.h>
#include <colinux/common/ioctl.h>
#include <colinux/os/user/manager.h>

#include "reactor.h"

typedef struct co_user_monitor {
	co_manager_handle_t handle;
	co_id_t 	    monitor_id;
	co_reactor_user_t   reactor_user;
} co_user_monitor_t;

extern co_rc_t co_user_monitor_create(co_user_monitor_t**	 out_mon,
				      co_manager_ioctl_create_t* params,
				      co_manager_handle_t	 handle);

extern co_rc_t co_user_monitor_open(co_reactor_t		   reactor,
				    co_reactor_user_receive_func_t receive,
				    co_id_t id, co_module_t*	   modules,
				    int				   num_modules,
				    co_user_monitor_t**		   out_mon);

extern co_rc_t co_user_monitor_load_section(co_user_monitor_t*		     umon,
					    co_monitor_ioctl_load_section_t* params);

extern co_rc_t co_user_monitor_load_initrd(co_user_monitor_t*	umon,
					   void*		initrd,
					   unsigned long	initrd_size);

extern co_rc_t co_user_monitor_run(co_user_monitor_t* umon, co_monitor_ioctl_run_t* params);
extern co_rc_t co_user_monitor_start(co_user_monitor_t* umon);

extern co_rc_t co_user_monitor_get_console(co_user_monitor_t*		   umon,
					   co_monitor_ioctl_get_console_t* params);

extern co_rc_t co_user_monitor_get_state(co_user_monitor_t*		 umon,
					   co_monitor_ioctl_get_state_t* params);

extern co_rc_t co_user_monitor_reset(co_user_monitor_t *umon);
extern co_rc_t co_user_monitor_status(co_user_monitor_t *umon,
				      co_monitor_ioctl_status_t *status);

extern co_rc_t co_user_monitor_message_send(co_user_monitor_t *umon,  co_message_t *message);

extern void co_user_monitor_close(co_user_monitor_t *umon);

extern co_id_t find_first_monitor(void);

extern co_rc_t read_pid_from_file(co_pathname_t pathname, co_id_t *id);

extern co_rc_t co_user_monitor_conet_bind_adapter(co_user_monitor_t *umon,
				co_monitor_ioctl_conet_bind_adapter_t *params);

extern co_rc_t co_user_monitor_conet_unbind_adapter(co_user_monitor_t *umon,
				co_monitor_ioctl_conet_unbind_adapter_t *params);
#endif
