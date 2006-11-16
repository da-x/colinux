/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_COBDPATH_H__
#define __COLINUX_OS_USER_COBDPATH_H__

#include <colinux/common/config.h>

extern co_rc_t co_canonize_cobd_path(co_pathname_t *pathname);
extern co_rc_t co_dirname (char *path);

#endif
