/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <windows.h>

extern "C" {
	#include <colinux/common/debug.h>
}

#include <colinux/user/console/main.h>

static char scancode_state[0x100];

static HHOOK current_hook;
static LRESULT CALLBACK keyboard_hook(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
)
{
	int AltDown = (lParam & (1 << 29)) ? 1 : 0;
	co_scan_code_t sc;
	sc.code = (lParam >> 16) & 0xff;
	sc.down = (lParam & (1 << 31)) ? 0 : 1;

	scancode_state[sc.code] = sc.down;

	co_user_console_handle_scancode(sc);

	if ((AltDown == 1) &&
            (wParam == 0x73)) {
		// Swallow the ALT-F4 keystroke.
		return 1;
	}

	return CallNextHookEx(current_hook, nCode, wParam, lParam);
}

void co_user_console_keyboard_focus_change(unsigned long keyboard_focus)
{
	if (keyboard_focus == 0) {
		int i;

		/*
		 * Lost keyboard focus. Release all pressed keys.
		 */ 

		for (i=0; i < 0x100; i++) {
			if (scancode_state[i]) {
				co_scan_code_t sc;
				sc.code = i;
				sc.down = 0;
				scancode_state[sc.code] = 0;

				co_user_console_handle_scancode(sc);
			}
		}
	}
}

int main(int argc, char **argv) 
{
	current_hook = SetWindowsHookEx(WH_KEYBOARD,
					keyboard_hook,
					NULL,
					GetCurrentThreadId());

	return co_user_console_main(argc, argv);
}
