/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_POLL_H__
#define __COLINUX_OS_USER_POLL_H__

#include <colinux/common/common.h>
#include <colinux/common/ioctl.h>

struct co_os_poll;
typedef struct co_os_poll *co_os_poll_t; 

struct co_os_poll_chain;
typedef struct co_os_poll_chain *co_os_poll_chain_t; 

typedef void (*co_os_poll_callback_t)(co_os_poll_t poll, void *data);

extern co_rc_t co_os_poll_chain_create(co_os_poll_chain_t *chain_out);
extern int co_os_poll_chain_wait(co_os_poll_chain_t chain);
extern void co_os_poll_chain_destroy(co_os_poll_chain_t chain);

extern co_rc_t co_os_poll_create(co_id_t id,
				 co_monitor_ioctl_op_t poll_op,
				 co_monitor_ioctl_op_t cancel_op,
				 co_os_poll_callback_t callback,
				 void *callback_data,
				 co_os_poll_chain_t chain,
				 co_os_poll_t *out_poll);

extern void co_os_poll_destroy(co_os_poll_t poll);

#endif
