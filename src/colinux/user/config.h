/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_CORE_CONFIG_H__
#define __COLINUX_CORE_CONFIG_H__

#include <colinux/common/common.h>
#include <colinux/common/config.h>

#include "macaddress.h"

extern co_rc_t co_parse_config_args(co_command_line_params_t cmdline, co_start_parameters_t *start_parameters);

#endif
