/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
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

typedef struct co_user_monitor {
	co_manager_handle_t handle;
	co_id_t monitor_id;
} co_user_monitor_t;

extern co_rc_t co_user_monitor_create(co_user_monitor_t **out_mon, 
				      co_manager_ioctl_create_t *params);
extern co_rc_t co_user_monitor_open(co_id_t id, co_user_monitor_t **out_mon);
extern void co_user_monitor_close(co_user_monitor_t *monitor);

extern co_rc_t co_user_monitor_any(co_user_monitor_t *monitor, co_monitor_ioctl_op_t op);


extern co_rc_t co_user_monitor_load_section(co_user_monitor_t *umon, 
					    co_monitor_ioctl_load_section_t *params);
extern co_rc_t co_user_monitor_run(co_user_monitor_t *umon);

extern co_rc_t co_user_monitor_get_console(co_user_monitor_t *umon, co_console_t **console);
extern co_rc_t co_user_monitor_console_messages(co_user_monitor_t *umon, 
						co_monitor_ioctl_console_messages_t *params,
						unsigned long size);
extern co_rc_t co_user_monitor_console_poll(co_user_monitor_t *umon);
extern co_rc_t co_user_monitor_console_cancel_poll(co_user_monitor_t *umon);
extern co_rc_t co_user_monitor_put_console(co_user_monitor_t *umon, co_console_t *console);

extern co_rc_t co_user_monitor_network_send(co_user_monitor_t *umon, 
					    char *data, unsigned long size);
extern co_rc_t co_user_monitor_network_receive(co_user_monitor_t *umon, 
					       char *data, unsigned long size);
extern co_rc_t co_user_monitor_network_poll(co_user_monitor_t *umon);
extern co_rc_t co_user_monitor_network_cancel_poll(co_user_monitor_t *umon);

extern co_rc_t co_user_monitor_keyboard(co_user_monitor_t *umon, 
					co_monitor_ioctl_keyboard_t *params);

extern co_rc_t co_user_monitor_status(co_user_monitor_t *umon, 
				      co_monitor_ioctl_status_t *status);
extern co_rc_t co_user_monitor_terminate(co_user_monitor_t *umon);

extern co_rc_t co_user_monitor_destroy(co_user_monitor_t *umon);

#endif
