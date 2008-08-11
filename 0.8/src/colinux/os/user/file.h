/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_FILE_H__
#define __COLINUX_OS_USER_FILE_H__

#include <colinux/common/common.h>

extern co_rc_t co_os_file_load(co_pathname_t pathname,
			       char **out_buf, unsigned long *out_size, unsigned long max_size);
extern void co_os_file_free(char *buf);

extern co_rc_t co_os_file_write(co_pathname_t pathname, void *buf, unsigned long size);
extern co_rc_t co_os_file_unlink(co_pathname_t pathname);

#endif
