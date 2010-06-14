/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_TAP_WIN32_H__
#define __CO_TAP_WIN32_H__

extern co_rc_t open_tap_win32(HANDLE *phandle, char *prefered_name);
extern BOOL tap_win32_set_status(HANDLE handle, BOOL status);

#endif
