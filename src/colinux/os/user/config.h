/*
 * This source code is a part of coLinux source package.
 *
 * Henry Nestler, 2008 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_CONFIG_H__
#define __COLINUX_OS_USER_CONFIG_H__

#if defined __cplusplus
extern "C" {
#endif

#include <colinux/common/common.h>

co_rc_t co_config_user_string_read(int		monitor_index,
				   const char*	device_name,
				   int		device_index,
				   const char*	value_name,
				   char*	value,
				   int 		size);

co_rc_t co_config_user_string_write(int		monitor_index,
				    const char* device_name,
				    int		device_index,
				    const char*	value_name,
				    const char*	value);

#if defined __cplusplus
}
#endif

#endif /* __COLINUX_OS_USER_CONFIG_H__ */
