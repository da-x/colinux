/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 * Ballard, Jonathan H.  <californiakidd@users.sourceforge.net>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>

#include "widget.h"

extern "C" {
#include <colinux/os/alloc.h>
#include <colinux/os/current/user/daemon.h>
}

console_widget_t *
co_console_widget_create()
{
	return new console_widget_NT_t();
}

console_widget_NT_t::console_widget_NT_t()
:  console_widget_t()
{
	allocatedConsole = false;
	keyed = 0;
	buffer = 0;
	screen = 0;
	input = 0;
	previous_output = 0;
}

co_rc_t
console_widget_NT_t::console_window(console_window_t * W)
{
	CONSOLE_CURSOR_INFO cci;

	window = W;

	if (GetConsoleWindow() == NULL) {
		AllocConsole();
		allocatedConsole = true;
	}

	input = GetStdHandle(STD_INPUT_HANDLE);
	previous_output = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleCursorInfo(previous_output, &previous_cursor);
	cci = previous_cursor;
	cci.bVisible = false;
	SetConsoleCursorInfo(previous_output, &cci);

	buffer =
	    CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, 0,
				      CONSOLE_TEXTMODE_BUFFER, 0);

	blank.Attributes = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED;
	blank.Char.AsciiChar = ' ';

	CONSOLE_SCREEN_BUFFER_INFO i;
	if (!GetConsoleScreenBufferInfo(buffer, &i))
		co_debug("GetConsoleScreenBufferInfo() error code %d\n",
			 GetLastError());

	size = i.dwSize;	// UPDATE: fixed 80x25 size; let console be variable size
//      region = i.srWindow ;

//      size.X = 80 ;
//      size.Y = 25 ;
	region.Top = 0;
	region.Left = 0;
	region.Right = 80;
	region.Bottom = 25;

	screen =
	    (CHAR_INFO *) co_os_malloc(sizeof (CHAR_INFO) * size.X * size.Y);

//      if( ! SetConsoleWindowInfo( buffer , TRUE , &region ) )
//              co_debug( "SetConsoleWindowInfo() error code %d\n" , GetLastError() ) ;
//      if( ! SetConsoleScreenBufferSize( buffer , size ) )
//              co_debug( "SetConsoleScreenBufferSize() error code %d\n" , GetLastError() ) ;

	SetConsoleMode(buffer, 0);

	SetConsoleMode(input, ENABLE_WINDOW_INPUT);

	cci.bVisible = true;
	cci.dwSize = 10;
	SetConsoleCursorInfo(buffer, &cci);

	SetConsoleActiveScreenBuffer(buffer);

	return CO_RC(OK);
}

console_widget_NT_t::~console_widget_NT_t()
{
	if (screen)
		co_os_free(screen);
	CloseHandle(buffer);
	SetConsoleCursorInfo(previous_output, &previous_cursor);
	if (allocatedConsole)
		FreeConsole();
}

void
console_widget_NT_t::draw()
{
	if (console == NULL) {
		COORD c = { 0, 0 };
		DWORD z;
		if (!FillConsoleOutputCharacter
		    (buffer, blank.Char.AsciiChar, size.X * size.Y, c, &z))
			co_debug("FillConsoleOutputCharacter() error code %d\n",
				 GetLastError());
		if (!FillConsoleOutputAttribute
		    (buffer, blank.Attributes, size.X * size.Y, c, &z))
			co_debug("FillConsoleOutputAttribute() error code %d\n",
				 GetLastError());
		return;
	}

	co_console_cell_t *cell = console->screen;
	CHAR_INFO *ci = screen;
	int count = size.X * size.Y;

	do {
		ci->Attributes = cell->attr;
		(ci++)->Char.AsciiChar = (cell++)->ch;
	} while (--count);
	co_debug("\n", cell->ch);

	SMALL_RECT r = region;
	COORD c = { 0, 0 };

	if (!WriteConsoleOutput(buffer, screen, size, c, &r))
		co_debug("WriteConsoleOutput() error %d \n", GetLastError());
}

co_rc_t
console_widget_NT_t::op_scroll(long &T, long &B, long &D, long &L)
{
	SMALL_RECT r;
	COORD c;

	r.Left = 0;
	r.Right = console->x;
	r.Top = T;
	r.Bottom = B;
	c.X = 0;
	c.Y = D == 1 ? r.Top - L : r.Top + L;

	if (!ScrollConsoleScreenBuffer(buffer, &r, &region, c, &blank))
		co_debug("ScrollConsoleScreenBuffer() error code: %d \n",
			 GetLastError());

	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_putcs(long &Y, long &X, char *D, long &C)
{
	int count;

	if ((count = C) <= 0)
		return CO_RC(ERROR);

	SMALL_RECT r;
	COORD c;

	c.X = r.Left = X;
	c.Y = r.Top = r.Bottom = Y;

	if (--count + r.Left > console->x)
		count = console->x - r.Left;
	r.Right = r.Left + count;

	co_console_cell_t *cells = (co_console_cell_t *) D;
	CHAR_INFO *ci = &screen[(size.X * r.Top) + r.Left];
	do {
		ci->Attributes = cells->attr;
		(ci++)->Char.AsciiChar = (cells++)->ch;
	} while (count--);

	if (!WriteConsoleOutput(buffer, screen, size, c, &r))
		co_debug("WriteConsoleOutput() error %d \n", GetLastError());
	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_putc(long &Y, long &X, unsigned short &C)
{
	SMALL_RECT r;
	COORD c;

	c.X = r.Left = r.Right = X;
	c.Y = r.Top = r.Bottom = Y;

	CHAR_INFO *ci = &screen[(size.X * r.Top) + r.Left];
	ci->Attributes = ((C & 0xFF00) >> 8);
	ci->Char.AsciiChar = (C & 0x00FF);

	if (!WriteConsoleOutput(buffer, screen, size, c, &r))
		co_debug("WriteConsoleOutput() error %d \n", GetLastError());
	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_cursor(co_cursor_pos_t & P)
{
	COORD c;
	c.X = P.x;
	c.Y = P.y;
	SetConsoleCursorPosition(buffer, c);
	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::loop()
{
	DWORD r;

	if (!GetNumberOfConsoleInputEvents(input, &r))
		return CO_RC(ERROR);
	if (r == 0)
		return CO_RC(OK);

	INPUT_RECORD i;
	ReadConsoleInput(input, &i, 1, &r);
	if (i.EventType == KEY_EVENT) {

		if (i.Event.KeyEvent.bKeyDown || keyed) {
			if (i.Event.KeyEvent.wVirtualKeyCode == 91
			    && i.Event.KeyEvent.
			    dwControlKeyState & ENHANCED_KEY) {
				keyed = 1;
			} else
			    if (i.Event.KeyEvent.wVirtualKeyCode == 92
				&& i.Event.KeyEvent.
				dwControlKeyState & ENHANCED_KEY) {
				keyed = 1;
			} else if (i.Event.KeyEvent.wVirtualKeyCode == 18 && i.Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED && keyed == 1) {	// window+alt key
				window->online(false);
				return CO_RC(OK);
			} else if (i.Event.KeyEvent.wVirtualKeyCode == 18 && i.Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED && keyed == 1) {	// window+alt key
				window->online(false);
				return CO_RC(OK);
			} else
				keyed = 0;
		} else
			keyed = 0;

		if (window->online()) {
			struct {
				co_message_t message;
				co_linux_message_t linux;
				co_scan_code_t code;
			} message;

			message.message.from = CO_MODULE_CONSOLE;
			message.message.to = CO_MODULE_LINUX;
			message.message.priority = CO_PRIORITY_DISCARDABLE;
			message.message.type = CO_MESSAGE_TYPE_OTHER;
			message.message.size =
			    sizeof (message) - sizeof (message.message);
			message.linux.device = CO_DEVICE_KEYBOARD;
			message.linux.unit = 0;
			message.linux.size = sizeof (message.code);
			message.code.code = i.Event.KeyEvent.wVirtualScanCode;
			message.code.down = i.Event.KeyEvent.bKeyDown;

			window->event(message.message);
		}
	}
	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::title(const char *T)
{
	SetConsoleTitle(T);
	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::idle()
{
	co_daemon_handle_t d = window->daemonHandle();

	if (d == static_cast < co_daemon_handle_t > (0))	// UPDATE: need a daemon_handle C++ class
	{
		WaitForSingleObject(input, INFINITE);
		return CO_RC(OK);
	}

	HANDLE h[2] = { input, d->readable };
	WaitForMultipleObjects(2, h, FALSE, INFINITE);
	return CO_RC(OK);
}
