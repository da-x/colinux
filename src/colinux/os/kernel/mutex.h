/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_KERNEL_MUTEX_H__
#define __CO_OS_KERNEL_MUTEX_H__

#include <colinux/common/common.h>

typedef struct co_os_mutex *co_os_mutex_t;

extern co_rc_t co_os_mutex_create(co_os_mutex_t *mutex_out);
extern void co_os_mutex_acquire(co_os_mutex_t mutex);
extern void co_os_mutex_acquire_critical(co_os_mutex_t mutex);
extern void co_os_mutex_release(co_os_mutex_t mutex);
extern void co_os_mutex_release_critical(co_os_mutex_t mutex);
extern void co_os_mutex_destroy(co_os_mutex_t mutex);

#endif
