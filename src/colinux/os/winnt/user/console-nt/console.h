/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 * Ballard, Jonathan H.  <californiakidd@users.sourceforge.net>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_WINNT_OS_USER_CONSOLE_WIDGET_H__
#define __COLINUX_WINNT_OS_USER_CONSOLE_WIDGET_H__

#include <windows.h>

#include <colinux/user/console-nt/console.h>

class console_window_NT_t : public console_window_t {
public:
	virtual const char* get_name();
};

#endif
