/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_KERNEL_WAIT_H__
#define __CO_OS_KERNEL_WAIT_H__

#include <colinux/common/common.h>

/*
 * Wait channels.
 */

typedef struct co_os_wait *co_os_wait_t;

extern co_rc_t co_os_wait_create(co_os_wait_t *wait_out);
extern void co_os_wait_sleep(co_os_wait_t wait);
extern void co_os_wait_wakeup(co_os_wait_t wait);
extern void co_os_wait_destroy(co_os_wait_t wait);

#endif
