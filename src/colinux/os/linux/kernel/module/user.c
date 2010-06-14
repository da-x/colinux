/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "linux_inc.h"

#include <colinux/os/kernel/user.h>

co_rc_t co_copy_to_user(char *user_address, char *kernel_address, unsigned long size)
{
	int ret;

	ret = copy_to_user(user_address, kernel_address, size);
	if (ret)
		return CO_RC(ERROR);

	return CO_RC(OK);;
}

co_rc_t co_copy_from_user(char *user_address, char *kernel_address, unsigned long size)
{
	int ret;

	ret = copy_from_user(user_address, kernel_address, size);
	if (ret)
		return CO_RC(ERROR);

	return CO_RC(OK);;
}
