/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_KERNEL_USER_H__
#define __CO_OS_KERNEL_USER_H__

#include <colinux/common/common.h>

/*
 * Userspace access routines.
 */

co_rc_t co_copy_to_user(char *user_address, char *kernel_address, unsigned long size);
co_rc_t co_copy_from_user(char *user_address, char *kernel_address, unsigned long size);

#endif
