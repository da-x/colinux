/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_OS_KERNEL_MANAGER_H__
#define __COLINUX_OS_KERNEL_MANAGER_H__

#include <colinux/kernel/manager.h>

extern co_rc_t co_os_manager_init(co_manager_t *manager, co_osdep_manager_t *osdep);
extern void co_os_manager_free(co_osdep_manager_t osdep);

extern co_rc_t co_os_manager_init(co_manager_t *manager, co_osdep_manager_t *osdep);
extern void co_os_manager_free(co_osdep_manager_t osdep);

extern co_rc_t co_os_manager_userspace_open(co_manager_open_desc_t opened);
extern void co_os_manager_userspace_close(co_manager_open_desc_t opened);

extern bool_t co_os_manager_userspace_try_send_direct(
	co_manager_t *manager,
	co_manager_open_desc_t opened,
	co_message_t *message);

extern co_rc_t co_os_manager_userspace_eof(
	co_manager_t *manager,
	co_manager_open_desc_t opened);

#endif
