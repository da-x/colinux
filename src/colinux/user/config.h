/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
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

co_rc_t co_load_config(char *text, co_config_t *out_config);

#endif
