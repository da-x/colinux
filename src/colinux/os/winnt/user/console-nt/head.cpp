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

#include <colinux/user/console-base/main.h>

COLINUX_DEFINE_MODULE("colinux-console-nt");

#if 0
static HHOOK current_hook;
static LRESULT CALLBACK
keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
	co_scan_code_t sc;
	sc.code = (lParam >> 16) & 0xff;
	sc.down = (lParam & (1 << 31)) ? 0 : 1;

	co_user_console_handle_scancode(sc);

	co_debug(" scancode...\n");

	return CallNextHookEx(current_hook, nCode, wParam, lParam);
}
#endif

int
main(int argc, char **argv)
{
	CONSOLE_CURSOR_INFO cursor;
        bool allocatedConsole = false;
	HANDLE output;
        HANDLE input;
        DWORD mode;
	int status;
	HWND hwnd;

	if ((hwnd = GetConsoleWindow())==(HWND)0) {
		AllocConsole();
		allocatedConsole = true;
		hwnd = GetConsoleWindow();
	}
	input = GetStdHandle(STD_INPUT_HANDLE);
	output = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleCursorInfo(output, &cursor);
	GetConsoleMode(input, &mode);

	try {
		status = co_user_console_main(argc, argv);
	} catch(...) {
		co_debug("The console program encountered an exception.\n");
		status = -1;
	}

	if(allocatedConsole) {
		FreeConsole();
		return status;
	}

	FlushConsoleInputBuffer(input);
	SetConsoleMode(input, mode);
	SetStdHandle(STD_INPUT_HANDLE, input);
	SetStdHandle(STD_OUTPUT_HANDLE, output);
	SetConsoleCursorInfo(output, &cursor);

	return status;
}
