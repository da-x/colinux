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
#include <colinux/user/console-base/main.h>

extern "C" {
#include <colinux/os/alloc.h>
#include <colinux/os/winnt/user/daemon.h>

	// Not found in w32api/mingw
extern PASCAL BOOL GetCurrentConsoleFont(HANDLE hConsoleOutput,
                                         BOOL bMaximumWindow,
                                         PCONSOLE_FONT_INFO lpCurrentFont);
extern PASCAL COORD GetConsoleFontSize(HANDLE hConsoleOutput,
                                       DWORD nFont);
}

static BOOL ctrl_exit;

/*
 * Storage of current keyboard state.
 * For every virtual key (256), it is 0 (for released) or the scancode
 * of the key pressed (with 0xE0 in high byte if extended).
 */
static WORD vkey_state[256];


console_widget_t *
co_console_widget_create()
{
	return new console_widget_NT_t();
}

BOOL WINAPI
co_console_widget_control_handler(DWORD T)
{
	DWORD r;
	INPUT_RECORD c;
	if (!(T == CTRL_CLOSE_EVENT || T == CTRL_LOGOFF_EVENT)) 
		return false;
	memset(&c, 0, sizeof(INPUT_RECORD));
	c.EventType = KEY_EVENT;
	WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &c, 1, &r);
	return ctrl_exit = true;
}

console_widget_NT_t::console_widget_NT_t()
:  console_widget_t()
{
	memset( vkey_state, 0, sizeof(vkey_state) );
	buffer = 0;
	screen = 0;
	input = 0;
	output = 0;
	blank.Attributes = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED;
	blank.Char.AsciiChar = ' ';
	ctrl_exit = false;
	SetConsoleCtrlHandler(co_console_widget_control_handler, true);
}

co_rc_t
console_widget_NT_t::console_window(console_window_t * W)
{
	CONSOLE_CURSOR_INFO cci;
	CONSOLE_FONT_INFO cfi;
	COORD fs;
	HWND hwnd;
	RECT r;

	window = W;

	input = GetStdHandle(STD_INPUT_HANDLE);
	SetConsoleMode(input, 0);

	output = GetStdHandle(STD_OUTPUT_HANDLE);
        GetCurrentConsoleFont(output, 0, &cfi);
        fs = GetConsoleFontSize(output, cfi.nFont);
        r.top = 0;
        r.left = 0;
        r.bottom = fs.Y * 25;
        r.right = fs.X * 80;
        AdjustWindowRect(&r, WS_CAPTION|WS_SYSMENU|WS_THICKFRAME
                             |WS_MINIMIZEBOX|WS_MAXIMIZEBOX, 0);

	hwnd = GetConsoleWindow();
	SetWindowPos(hwnd, HWND_TOP, 0, 0,
                     r.right - r.left, r.bottom - r.top,
                     SWP_NOMOVE|SWP_SHOWWINDOW);

        GetConsoleCursorInfo(output, &cursor);
        cci = cursor;
        cci.bVisible = 0;
        SetConsoleCursorInfo(output, &cci);

	size.X = 80 ;
        size.Y = 25 ;
	region.Top = 0;
	region.Left = 0;
        region.Right = 79;
        region.Bottom = 24;

        if( ! SetConsoleWindowInfo( output , TRUE , &region ) )
         co_debug("SetConsoleWindowInfo() error code %d\n", GetLastError());

	screen =
	    (CHAR_INFO *) co_os_malloc(sizeof (CHAR_INFO) * size.X * size.Y);

	buffer =
	    CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, 0,
				      CONSOLE_TEXTMODE_BUFFER, 0);

	if( ! SetConsoleScreenBufferSize( buffer , size ) )
          co_debug("SetConsoleScreenBufferSize() error %d\n", GetLastError());

	SetConsoleMode(buffer, 0);

	cci.bVisible = true;
	cci.dwSize = 10;
	SetConsoleCursorInfo(buffer, &cci);

	SetConsoleActiveScreenBuffer(buffer);

	return CO_RC(OK);
}

console_widget_NT_t::~console_widget_NT_t()
{
	SetConsoleCtrlHandler(co_console_widget_control_handler, false);
	if (screen)
		co_os_free(screen);
	CloseHandle(buffer);
        SetConsoleCursorInfo(output, &cursor);
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

	SMALL_RECT r = region;
	COORD c = { 0, 0 };

	if (!WriteConsoleOutput(buffer, screen, size, c, &r))
		co_debug("WriteConsoleOutput() error %d \n", GetLastError());
}

void
console_widget_NT_t::co_console_update()
{
	SMALL_RECT r = region;
	COORD c = { 0, 0 };

	if (!ReadConsoleOutput(buffer, screen, size, c, &r))
		co_debug("ReadConsoleOutput() error %d \n", GetLastError());

	co_console_cell_t *cell = console->screen;
	CHAR_INFO *ci = screen;
	int count = size.X * size.Y;

	do {
		cell->attr = ci->Attributes;
		(cell++)->ch = (ci++)->Char.AsciiChar;
	} while (--count);
}

co_rc_t
console_widget_NT_t::op_scroll_up(
		const co_console_unit &T,
		const co_console_unit &B,
		const co_console_unit &L)
{
	SMALL_RECT r;
	COORD c;

	r.Left = region.Left;
	r.Right = region.Right;
	r.Top = T;
	r.Bottom = B;
	c.X = 0;
	c.Y = r.Top - L;

	if (!ScrollConsoleScreenBuffer(buffer, &r, &r, c, &blank))
		co_debug("ScrollConsoleScreenBuffer() error code: %d \n",
			 GetLastError());

	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_scroll_down(
		const co_console_unit &T,
		const co_console_unit &B,
		const co_console_unit &L)
{
	SMALL_RECT r;
	COORD c;

	r.Left = region.Left;
	r.Right = region.Right;
	r.Top = T;
	r.Bottom = B;
	c.X = 0;
	c.Y = r.Top + L;

	if (!ScrollConsoleScreenBuffer(buffer, &r, &r, c, &blank))
		co_debug("ScrollConsoleScreenBuffer() error code: %d \n",
			 GetLastError());

	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_putcs(
		const co_console_unit &Y,
		const co_console_unit &X,
		const co_console_character *D,
		const co_console_unit &C)
{
	int count;

	if ((count = C) <= 0)
		return CO_RC(ERROR);

	if(Y >= size.Y || X >= size.X) {
		co_debug("putc error\n");
		return CO_RC(ERROR);
	}

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
console_widget_NT_t::op_putc(
		const co_console_unit &Y,
		const co_console_unit &X,
		const co_console_character &C)
{
	SMALL_RECT r;
	COORD c;

	if(Y >= size.Y || X >= size.X) {
		co_debug("putc error\n");
		return CO_RC(ERROR);
	}

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
console_widget_NT_t::op_cursor(
		const co_cursor_pos_t & P)
{
	COORD c;
	c.X = P.x;
	c.Y = P.y;
	SetConsoleCursorPosition(buffer, c);
	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_clear(
		const co_console_unit &T,
		const co_console_unit &L,
		const co_console_unit &B,
		const co_console_unit &R,
		const co_console_character charattr)
{
	SMALL_RECT r;
	CHAR_INFO *s;
	CHAR_INFO clear_blank;
	COORD c;
	long y, x;

	if( B < T || R < L )
		return CO_RC(ERROR);
	if( T >= size.Y || L >= size.X )
		return CO_RC(ERROR);
	if( B >= size.Y || R >= size.X )
		return CO_RC(ERROR);

	clear_blank.Attributes = charattr >> 8;
	clear_blank.Char.AsciiChar = charattr & 0xff;

	y = T;
	while(y <= B) {
		s = &screen[(size.X * y) + (x = L)];
		while(x++ <= R)
			*(s++) = clear_blank;
		y++;
	}

	r.Top = c.Y = T;
	r.Left = c.X = L;
	r.Bottom = B;
	r.Right = R;
	
	if (!WriteConsoleOutput(buffer, screen, size, c, &r))
		co_debug("WriteConsoleOutput() error %d \n", GetLastError());

	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_bmove(
		const co_console_unit &Y,
		const co_console_unit &X,
		const co_console_unit &T,
		const co_console_unit &L,
		const co_console_unit &B,
		const co_console_unit &R)
{
	SMALL_RECT r;
	COORD c;

	c.Y = Y;
	c.X = X;

	r.Top = T;
	r.Left = L;
	r.Bottom = B;
	r.Right = R;

	if(!ScrollConsoleScreenBuffer(buffer, &r, &region, c, &blank))
		co_debug("ScrollConsoleScreenBuffer() error %d\n", GetLastError());
	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_invert(
		const co_console_unit &Y,
		const co_console_unit &X,
		const co_console_unit &C)
{
#if 0
	SMALL_RECT r = region;
	COORD c = { 0, 0 };

	co_debug("invert (%d,%d,%d)\n",Y,X,C);

	if (!ReadConsoleOutput(buffer, screen, size, c, &r))
		co_debug("ReadConsoleOutput() error %d \n", GetLastError());

	CHAR_INFO *ci = &screen[(Y*size.X)+X];
	unsigned a;
	unsigned count = C;

	while(count--) {
		a = ci->Attributes;
		ci->Attributes = (((a) & 0x70) >> 4) | (((a) & 0x07) << 4);
	}

	r = region;

	if (!WriteConsoleOutput(buffer, screen, size, c, &r))
		co_debug("WriteConsoleOutput() error %d \n", GetLastError());
#endif
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
	if(ctrl_exit) {
		window->online(false);
		return CO_RC(OK);
	}
	switch ( i.EventType )
	{
	case KEY_EVENT:
		ProcessKeyEvent( i.Event.KeyEvent );
		break;
	case FOCUS_EVENT:
		/* MSDN says this events should be ignored ??? */
		ProcessFocusEvent( i.Event.FocusEvent );
		break;
	case MOUSE_EVENT:
		/* *TODO: must be enabled first also */
		break;
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

void send_key( DWORD code )
{
	co_scan_code_t sc;
	sc.down = 1;	/* to work with old kernels that don't ignore this field */
	/* send e0 if extended key */
	if ( code & 0xE000 )
	{
		sc.code = 0xE0;
		co_user_console_handle_scancode( sc );
	}
	sc.code = code & 0xFF;
	co_user_console_handle_scancode( sc );
}

void console_widget_NT_t::ProcessKeyEvent( KEY_EVENT_RECORD& ker )
{
	const BYTE vkey     = static_cast<BYTE>( ker.wVirtualKeyCode );
	const WORD flags    = ker.dwControlKeyState;
	const bool released = (ker.bKeyDown == FALSE);
	const bool extended = flags & ENHANCED_KEY;
	WORD       code     = ker.wVirtualScanCode;

	/* Special key processing */
	switch ( vkey )
	{
	case VK_LWIN:
	case VK_RWIN:
		// Check if LeftAlt+Win (detach from colinux)
		if ( !released && (flags & LEFT_ALT_PRESSED) )
		{
			window->online(false);
			return;
		}
		// Signal Win key pressed/released
		if ( released )	vkey_state[255] &= ~1;
		else			vkey_state[255] |=  1;
	case VK_APPS:	// Window Context Menu
		return;		// Let windows process this keys
	case VK_MENU:
		// Check if Win+LeftAlt (detach from colinux)
		if ( (vkey_state[255] & 1) && !released )
		{
			window->online(false);
			return;
		}
		break;
	}

	/* Normal key processing */
	if ( extended )
		code |= 0xE000; /* extended scan-code code */
	if ( released )
	{	/* key was released */
		code |= 0x80;
		if ( vkey_state[vkey] == 0 )
			return;		/* ignore release of not pressed keys */
		vkey_state[vkey] = 0;
	}
	else
	{	/* Key pressed */
		vkey_state[vkey] = code | 0x80;
	}

	/* Send key scancode */
	send_key( code );

	return;
}

/*
 * console_widget_NT_t::ProcessFocusEvent
 *
 * MSDN says this event is used internally only and should be ignored.
 * But it seems to work ok, at least on XP ???
 * I believe a broken focus handler is better than nothing, so here it is.
 */
void console_widget_NT_t::ProcessFocusEvent( FOCUS_EVENT_RECORD& fer )
{
	if ( ! fer.bSetFocus )
	{
		for ( int i = 0; i < 255; ++i )
			if ( vkey_state[i] )
			{	// release it
				send_key( vkey_state[i] );
				vkey_state[i] = 0;
			}
		vkey_state[255] = 0;
	}
}
