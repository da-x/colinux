/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_MANAGER_H__
#define __COLINUX_OS_USER_MANAGER_H__

#if defined __cplusplus
extern "C" {
#endif

#include <colinux/common/common.h>
#include <colinux/os/kernel/manager.h>
#include <colinux/user/reactor.h>

struct co_manager_handle;
typedef struct co_manager_handle* co_manager_handle_t;

extern co_manager_handle_t co_os_manager_open(void);
extern co_manager_handle_t co_os_manager_open_quite(void);
extern void co_os_manager_close(co_manager_handle_t handle);

extern co_rc_t co_os_manager_ioctl(
	co_manager_handle_t 	kernel_device,
	unsigned long 		code,
	void*			input_buffer,
	unsigned long		input_buffer_size,
	void*			output_buffer,
	unsigned long		output_buffer_size,
	unsigned long*		output_returned);

extern co_rc_t co_os_manager_is_installed(bool_t* installed);


extern co_rc_t co_os_reactor_monitor_create(
	co_reactor_t 			reactor,
	co_manager_handle_t 		whandle,
	co_reactor_user_receive_func_t	receive,
	co_reactor_user_t*		handle_out);

extern void co_os_reactor_monitor_destroy(co_reactor_user_t handle);

#if defined __cplusplus
}
#endif

#endif /* __COLINUX_OS_USER_MANAGER_H__ */
