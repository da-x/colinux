/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_USER_MACADDRESS_H__
#define __COLINUX_USER_MACADDRESS_H__

#include <colinux/common/common.h>

extern co_rc_t co_parse_mac_address(const char *text, unsigned char *binary);
extern void co_build_mac_address(char *text, int ntext, const unsigned char *mac);

#endif
