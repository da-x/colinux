/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 * Service support by Jaroslaw Kowalski <jaak@zd.com.pl>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <unistd.h>

#include <colinux/common/libc.h>
#include <colinux/common/common.h>
#include <colinux/os/user/cobdpath.h>

co_rc_t co_canonize_cobd_path(co_pathname_t *pathname)
{
	co_pathname_t copied_path;
	co_pathname_t cwd_path;

	memcpy(&copied_path, pathname, sizeof(copied_path));

	if (co_strncmp(copied_path, "\\Device\\", 8) != 0) {
		if (co_strlen(copied_path) >= 3) {
			if (copied_path[1] == ':' && copied_path[2] == '\\') {
				co_snprintf(*pathname, sizeof(*pathname), "\\DosDevices\\%s", copied_path);
				return CO_RC(OK);
			}
		} 
		
		getcwd(cwd_path, sizeof(cwd_path));
		co_snprintf(*pathname, sizeof(*pathname), "\\DosDevices\\%s\\%s", cwd_path, copied_path);
	}

	return CO_RC(OK);
}
