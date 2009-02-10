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
	allocated_console = false;
	init_new_console();

	memset(vkey_state, 0, sizeof(vkey_state));
	buffer = 0;
	screen = 0;
	input = 0;
	output = 0;
	blank.Attributes = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED;
	blank.Char.AsciiChar = ' ';
	ctrl_exit = false;
	SetConsoleCtrlHandler(co_console_widget_control_handler, true);
}

void console_widget_NT_t::init_new_console()
{	
	allocated_console = false;

	saved_hwnd = GetConsoleWindow();
	if (saved_hwnd == (HWND)0) {
		AllocConsole();
		allocated_console = true;
		saved_hwnd = GetConsoleWindow();
	}

	saved_input = GetStdHandle(STD_INPUT_HANDLE);
	saved_output = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleCursorInfo(saved_output, &saved_cursor);
	GetConsoleMode(saved_input, &saved_mode);
}

void console_widget_NT_t::restore_console()
{
	FlushConsoleInputBuffer(saved_input);
	SetConsoleMode(saved_input, saved_mode);
	SetStdHandle(STD_INPUT_HANDLE, saved_input);
	SetStdHandle(STD_OUTPUT_HANDLE, saved_output);
	SetConsoleCursorInfo(saved_output, &saved_cursor);

	if (allocated_console) {
		FreeConsole();
	}
}

co_rc_t
console_widget_NT_t::set_window(console_window_t * W)
{
	CONSOLE_CURSOR_INFO cci;
	CONSOLE_FONT_INFO cfi;
	COORD fs;
	HWND hwnd;
	RECT r;
	DWORD error = 0;

	window = W;

	if (screen) {
    		co_debug("always screen %p", screen);
		return CO_RC(OK);
	}

	if (!console) {
    		co_debug("Bug: console = NULL");
		return CO_RC(ERROR);
	}

	size.X = console->x;
	size.Y = console->y;

	input = GetStdHandle(STD_INPUT_HANDLE);
	SetConsoleMode(input, 0);

	output = GetStdHandle(STD_OUTPUT_HANDLE);
        GetCurrentConsoleFont(output, 0, &cfi);
        fs = GetConsoleFontSize(output, cfi.nFont);
        r.top = 0;
        r.left = 0;
        r.bottom = fs.Y * size.Y;
        r.right = fs.X * size.X;
        AdjustWindowRect(&r, WS_CAPTION|WS_SYSMENU|WS_THICKFRAME
                             |WS_MINIMIZEBOX|WS_MAXIMIZEBOX, 0);

	hwnd = GetConsoleWindow();
	SetWindowPos(hwnd, HWND_TOP, 0, 0,
                     r.right - r.left, r.bottom - r.top,
                     SWP_NOMOVE);

        GetConsoleCursorInfo(output, &cursor);
        cci = cursor;
        cci.bVisible = 0;
        SetConsoleCursorInfo(output, &cci);

        region.Top = 0;
        region.Left = 0;
        region.Right = size.X-1;
        region.Bottom = size.Y-1;

	if( ! SetConsoleWindowInfo( output , TRUE , &region ) ) {
		error = GetLastError();
		co_debug("SetConsoleWindowInfo() error 0x%lx", error);
	}

	screen =
	    (CHAR_INFO *) co_os_malloc(sizeof (CHAR_INFO) * size.X * size.Y);

	buffer =
	    CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, 0,
				      CONSOLE_TEXTMODE_BUFFER, 0);

	if( ! SetConsoleScreenBufferSize( buffer , size ) )
          co_debug("SetConsoleScreenBufferSize() error 0x%lx", GetLastError());

	SetConsoleMode(buffer, 0);

	cci.bVisible = true;
	cci.dwSize = 10;
	SetConsoleCursorInfo(buffer, &cci);

	SetConsoleActiveScreenBuffer(buffer);

	// Fixup, if resize failed from smaller start window
	if (error == ERROR_INVALID_PARAMETER) {
		SetWindowPos(hwnd, HWND_TOP, 0, 0,
                	r.right - r.left + GetSystemMetrics(SM_CYVSCROLL),
			r.bottom - r.top + GetSystemMetrics(SM_CYHSCROLL),
                	SWP_NOMOVE);
	}

	return CO_RC(OK);
}

console_widget_NT_t::~console_widget_NT_t()
{
	SetConsoleCtrlHandler(co_console_widget_control_handler, false);
	if (screen)
		co_os_free(screen);
	CloseHandle(buffer);
        SetConsoleCursorInfo(output, &cursor);

	restore_console();
}

void
console_widget_NT_t::draw()
{
	if (console == NULL) {
		COORD c = {0, 0};
		DWORD z;
		if (!FillConsoleOutputCharacter
		    (buffer, blank.Char.AsciiChar, size.X * size.Y, c, &z))
			co_debug("FillConsoleOutputCharacter() error 0x%lx",
				 GetLastError());
		if (!FillConsoleOutputAttribute
		    (buffer, blank.Attributes, size.X * size.Y, c, &z))
			co_debug("FillConsoleOutputAttribute() error 0x%lx",
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
	COORD c = {0, 0};

	if (!WriteConsoleOutput(buffer, screen, size, c, &r))
		co_debug("WriteConsoleOutput() error 0x%lx", GetLastError());
}

void
console_widget_NT_t::update()
{
	SMALL_RECT r = region;
	COORD c = { 0, 0 };

	if (!ReadConsoleOutput(buffer, screen, size, c, &r))
		co_debug("ReadConsoleOutput() error 0x%lx", GetLastError());

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
		co_debug("ScrollConsoleScreenBuffer() error 0x%lx",
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
		co_debug("ScrollConsoleScreenBuffer() error 0x%lx",
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
		co_debug("putc error");
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
		co_debug("WriteConsoleOutput() error 0x%lx", GetLastError());

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

	if (Y >= size.Y || X >= size.X) {
		co_debug("putc error");
		return CO_RC(ERROR);
	}

	c.X = r.Left = r.Right = X;
	c.Y = r.Top = r.Bottom = Y;

	CHAR_INFO *ci = &screen[(size.X * r.Top) + r.Left];
	ci->Attributes = ((C & 0xFF00) >> 8);
	ci->Char.AsciiChar = (C & 0x00FF);

	if (!WriteConsoleOutput(buffer, screen, size, c, &r))
		co_debug("WriteConsoleOutput() error 0x%lx", GetLastError());
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

	if (B < T || R < L)
		return CO_RC(ERROR);
	if (T >= size.Y || L >= size.X)
		return CO_RC(ERROR);
	if (B >= size.Y || R >= size.X)
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
		co_debug("WriteConsoleOutput() error 0x%lx", GetLastError());

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
		co_debug("ScrollConsoleScreenBuffer() error 0x%lx", GetLastError());

	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_invert(
		const co_console_unit &Y,
		const co_console_unit &X,
		const co_console_unit &C)
{
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
	if (ctrl_exit) {
		window->detach();
		return CO_RC(OK);
	}

	switch ( i.EventType )
	{
	case KEY_EVENT:
		process_key_event(i.Event.KeyEvent);
		break;
	case FOCUS_EVENT:
		/* MSDN says this events should be ignored ??? */
		process_focus_event(i.Event.FocusEvent);
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

void console_widget_NT_t::process_key_event( KEY_EVENT_RECORD& ker )
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
	case VK_RWIN: {

		// Check if LeftAlt+Win (detach from colinux)
		if (!released && (flags & LEFT_ALT_PRESSED)) {

			// Release LeftALT before exit
			send_key(0x38|0x80);

			window->detach();
			return;
		}

		// Signal Win key pressed/released
		if (released)	
			vkey_state[255] &= ~1;
		else	
			vkey_state[255] |=  1;

		break;
	}

	case VK_DELETE: {
		if (!released  && 
		    ((flags & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) &&
		     (flags & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) &&
		     (flags & (SHIFT_PRESSED))))
		{
			window->send_ctrl_alt_del();
			return;
		}

		break;
	}

	case VK_APPS:	// Window Context Menu
		return;		// Let windows process this keys

	case VK_MENU: // Left ALT pressed
		// Check if Win+LeftAlt (detach from colinux)
		if ((vkey_state[255] & 1) && !released) {

			// Release WinKey before exit
			send_key(0xE05B|0x80);

			window->detach();
			return;
		}
		break;
	default:
		// Handle sticky mode keys after Alt+Space combination
		if (!released && !(flags & (RIGHT_ALT_PRESSED | SHIFT_PRESSED))) {
			// Release hanging ALT
			if (vkey_state[0x12]) {
				send_key(vkey_state[0x12]);
				vkey_state[0x12] = 0;
			}
			// Release hanging SHIFT
			if (vkey_state[0x10]) {
				send_key(vkey_state[0x10]);
				vkey_state[0x10] = 0;
			}
		}
	}

	/* Normal key processing */
	if (extended)
		code |= 0xE000; /* extended scan-code code */

	if (released) {	/* key was released */
		code |= 0x80;
		if ( vkey_state[vkey] == 0 )
			return;		/* ignore release of not pressed keys */
		vkey_state[vkey] = 0;
	} else { /* Key pressed */
		vkey_state[vkey] = code | 0x80;
	}

	/* Send key scancode */
	send_key(code);

	return;
}

/*
 * console_widget_NT_t::process_focus_event
 *
 * MSDN says this event is used internally only and should be ignored.
 * But it seems to work ok, at least on XP ???
 * I believe a broken focus handler is better than nothing, so here it is.
 */
void console_widget_NT_t::process_focus_event( FOCUS_EVENT_RECORD& fer )
{
	if (fer.bSetFocus)
		return;

	for (int i = 0; i < 255; ++i) {
		if (vkey_state[i]) {	// release it
			send_key(vkey_state[i]);
			vkey_state[i] = 0;
		}
	}

	vkey_state[255] = 0;
}
