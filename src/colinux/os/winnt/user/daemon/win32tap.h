/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __CO_WIN32_TAP_H__
#define __CO_WIN32_TAP_H__

co_rc_t open_win32_tap(HANDLE *phandle);
BOOL win32_tap_set_status(HANDLE handle, BOOL status);

#endif
