/*
 * This source code is a part of coLinux source package.
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <colinux/os/user/config.h>
#include <colinux/os/user/misc.h>

co_rc_t co_config_user_string_read(int monitor_index, const char *device_name, int device_index, const char *value_name, char *value, int size)
{
	return CO_RC(ERROR);
}

co_rc_t co_config_user_string_write(int monitor_index, const char *device_name, int device_index, const char *value_name, const char *value)
{
	return CO_RC(ERROR);
}
