/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_USER_CONSOLE_MAIN_H__
#define __COLINUX_USER_CONSOLE_MAIN_H__

#include <colinux/common/common.h>
#include "console.h"

enum REGISTRY_PARAMS { REGISTRY_FONT, REGISTRY_FONT_SIZE, REGISTRY_COPYSPACES,
	REGISTRY_EXITDETACH };
extern int co_user_console_main(int argc, char **argv);
extern void co_user_console_handle_scancode(co_scan_code_t sc);
extern void co_user_console_keyboard_focus_change(unsigned long keyboard_focus);
console_window_t* co_user_console_get_window(void);
extern int PasteClipboardIntoColinux(void);
extern int CopyLinuxIntoClipboard(void);
extern int ReadRegistry(int key);
extern int WriteRegistry(int key, int new_value);

#endif
