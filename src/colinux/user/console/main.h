/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#ifndef __COLINUX_USER_CONSOLE_MAIN_H__
#define __COLINUX_USER_CONSOLE_MAIN_H__

extern "C" {
#include <colinux/common/keyboard.h>
}

extern int co_user_console_main(int argc, char **argv);
extern void co_user_console_handle_scancode(co_scan_code_t sc);

#endif
