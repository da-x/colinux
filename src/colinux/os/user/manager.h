/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_MANAGER_H__
#define __COLINUX_OS_USER_MANAGER_H__

#include <colinux/common/common.h>

struct co_manager_handle;
typedef struct co_manager_handle *co_manager_handle_t; 

extern co_manager_handle_t co_os_manager_open(void);
extern void co_os_manager_close(co_manager_handle_t handle);

extern co_rc_t co_os_manager_ioctl(
	co_manager_handle_t kernel_device,
	unsigned long code,
	void *input_buffer,
	unsigned long input_buffer_size,
	void *output_buffer,
	unsigned long output_buffer_size,
	unsigned long *output_returned);

extern co_rc_t co_os_manager_remove(void);
extern co_rc_t co_os_manager_install(void);

#endif
