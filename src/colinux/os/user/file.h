/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_FILE_H__
#define __COLINUX_OS_USER_FILE_H__

#include <colinux/common/common.h>

extern co_rc_t co_os_file_load(co_pathname_t *pathname, 
			       char **out_buf, unsigned long *out_size);
extern void co_os_file_free(char *buf);

#endif
