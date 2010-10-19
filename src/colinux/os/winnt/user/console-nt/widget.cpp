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

 /*
  * WinNT dependent widget class
  * Used for building colinux-console-nt.exe
  */

#include <windows.h>

#include "widget.h"
#include "os_console.h"

#include <colinux/user/console-nt/main.h>

extern "C" {
#include <colinux/os/alloc.h>

// Not found in w32api/mingw
extern PASCAL BOOL GetCurrentConsoleFont(HANDLE             hConsoleOutput,
                                         BOOL               bMaximumWindow,
                                         PCONSOLE_FONT_INFO lpCurrentFont);

extern PASCAL COORD GetConsoleFontSize(HANDLE hConsoleOutput,
                                       DWORD  nFont);
}

static BOOL ctrl_exit;

/*
 * Storage of current keyboard state.
 * For every virtual key (256), it is 0 (for released) or the scancode
 * of the key pressed (with 0xE0 in high byte if extended).
 */
static WORD vkey_state[256];


console_widget_t* co_console_widget_create()
{
	return new console_widget_NT_t();
}

BOOL WINAPI co_console_widget_control_handler(DWORD T)
{
	DWORD        r;
	INPUT_RECORD c;

	if (!(T == CTRL_CLOSE_EVENT || T == CTRL_LOGOFF_EVENT))
		return false;

	memset(&c, 0, sizeof(INPUT_RECORD));
	c.EventType = KEY_EVENT;
	WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &c, 1, &r);
	return ctrl_exit = true;
}

console_widget_NT_t::console_widget_NT_t()
: console_widget_t()
{
	allocated_console = false;
	own_out_h         = 0;
	std_out_h         = 0;
	input             = 0;
	screen            = NULL;

	save_standard_console();

	memset(vkey_state, 0, sizeof(vkey_state));

	blank.Attributes = FOREGROUND_GREEN |
			   FOREGROUND_BLUE  |
			   FOREGROUND_RED;

	blank.Char.AsciiChar = ' ';
	ctrl_exit            = false;
	SetConsoleCtrlHandler(co_console_widget_control_handler, true);
}

void console_widget_NT_t::save_standard_console()
{
	saved_hwnd = GetConsoleWindow();
	if (saved_hwnd == (HWND)0) {
		AllocConsole();
		allocated_console = true;
		saved_hwnd        = GetConsoleWindow();
	}

	saved_input  = GetStdHandle(STD_INPUT_HANDLE);
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

co_rc_t console_widget_NT_t::set_window(console_window_t* W)
{
	CONSOLE_CURSOR_INFO	cci;
	CONSOLE_FONT_INFO	cfi;
	COORD			fs;
	HWND			hwnd;
	RECT			r;
	DWORD			error;
	DWORD			old_curs_size;
	BOOL			old_curs_is_vis;

	error  = 0;
	window = W;

	if (screen != NULL) {
    		co_debug("always screen %p", screen);
		return CO_RC(OK);
	}

	if (console == NULL) {
    		co_debug("Bug: console = NULL");
		return CO_RC(ERROR);
	}

	win_size.X = console->config.x;
	win_size.Y = console->config.y;

	input = GetStdHandle(STD_INPUT_HANDLE);
	SetConsoleMode(input, 0);

	std_out_h = GetStdHandle(STD_OUTPUT_HANDLE);
	GetCurrentConsoleFont(std_out_h, 0, &cfi);
	fs       = GetConsoleFontSize(std_out_h, cfi.nFont);
	r.top    = 0;
	r.left   = 0;
	r.bottom = fs.Y * win_size.Y;
	r.right  = fs.X * win_size.X;
	AdjustWindowRect(&r,
			 WS_CAPTION     |
			 WS_SYSMENU     |
			 WS_THICKFRAME  |
			 WS_MINIMIZEBOX |
			 WS_MAXIMIZEBOX,
			 0);

	hwnd = GetConsoleWindow();
	SetWindowPos(hwnd,
		     HWND_TOP,
		     0,
		     0,
		     r.right  - r.left,
		     r.bottom - r.top,
		     SWP_NOMOVE);

	// Hide the standard screen cursor
        GetConsoleCursorInfo(std_out_h, &cursor_info);
	old_curs_is_vis = cci.bVisible;
	old_curs_size   = cci.dwSize;
	cci             = cursor_info;
	cci.bVisible    = FALSE;
	SetConsoleCursorInfo(std_out_h, &cci);

	/* Window region */
	win_region.Top    = 0;
	win_region.Left   = 0;
	win_region.Right  = win_size.X - 1;
	win_region.Bottom = win_size.Y - 1;

	if(!SetConsoleWindowInfo(std_out_h, TRUE, &win_region)) {
		error = GetLastError();
		co_debug("SetConsoleWindowInfo() error 0x%lx", error);
	}

	buf_size = win_size;

#if CO_ENABLE_CON_SCROLL
	if(console->config.max_y > buf_size.Y)
		buf_size.Y = console->config.max_y;
#endif

	screen = (CHAR_INFO*)co_os_malloc(sizeof(CHAR_INFO) * buf_size.X * buf_size.Y);

	own_out_h = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
					      0,
					      0,
					      CONSOLE_TEXTMODE_BUFFER,
					      0);

	if(!SetConsoleScreenBufferSize(own_out_h, buf_size))
          	co_debug("SetConsoleScreenBufferSize() error 0x%lx", GetLastError());

	SetConsoleMode(own_out_h, 0);

	// Show the standard screen cursor
	if(old_curs_is_vis) {
		cci.bVisible = true;
		cci.dwSize   = old_curs_size;
		SetConsoleCursorInfo(own_out_h, &cci);
	}

	SetConsoleActiveScreenBuffer(own_out_h);

	/* Set cursor size for own screen buffer */
	console->cursor.height = console->config.curs_type_size;
	set_cursor_size(console->cursor.height);

	// Fixup, if resize failed from smaller start window
	if (error == ERROR_INVALID_PARAMETER) {
		SetWindowPos(hwnd,
			     HWND_TOP,
			     0,
			     0,
			     r.right  - r.left + GetSystemMetrics(SM_CYVSCROLL),
			     r.bottom - r.top  + GetSystemMetrics(SM_CYHSCROLL),
			     SWP_NOMOVE);
	}

	return CO_RC(OK);
}

console_widget_NT_t::~console_widget_NT_t()
{
	/* Restore standard Ctrl Break handler */
	SetConsoleCtrlHandler(co_console_widget_control_handler, false);
	if (screen != NULL)
		co_os_free(screen);

	/* Restore standard output screen buffer */
	CloseHandle(own_out_h);
	SetConsoleCursorInfo(std_out_h, &cursor_info);

	restore_console();
}

void console_widget_NT_t::draw()
{
	if (console == NULL) {
		COORD c = {0, 0};
		DWORD z;

		if (!FillConsoleOutputCharacter(own_out_h,
						blank.Char.AsciiChar,
						win_size.X * win_size.Y,
						c,
						&z))
			co_debug("FillConsoleOutputCharacter() error 0x%lx",
				 GetLastError());
		if (!FillConsoleOutputAttribute(own_out_h,
						blank.Attributes,
						win_size.X * win_size.Y,
						c,
						&z))
			co_debug("FillConsoleOutputAttribute() error 0x%lx",
				 GetLastError());
		return;
	}

	co_console_cell_t*	cell  = console->screen;
	CHAR_INFO*		ci    = screen;
	int			count = win_size.X * win_size.Y;

	do {
		ci->Attributes         = cell->attr;
		(ci++)->Char.AsciiChar = (cell++)->ch;
	} while (--count);

	SMALL_RECT r = win_region;
	COORD      c = {0, 0};

	if(!WriteConsoleOutput(own_out_h, screen, win_size, c, &r))
		co_debug("WriteConsoleOutput() error 0x%lx", GetLastError());
}

void console_widget_NT_t::update()
{
	SMALL_RECT r = win_region;
	COORD      c = {0, 0};

	if (!ReadConsoleOutput(own_out_h, screen, win_size, c, &r))
		co_debug("ReadConsoleOutput() error 0x%lx", GetLastError());

	co_console_cell_t*	cell  = console->screen;
	CHAR_INFO*		ci    = screen;
	int			count = win_size.X * win_size.Y;

	do {
		cell->attr   = ci->Attributes;
		(cell++)->ch = (ci++)->Char.AsciiChar;
	} while(--count != 0);
}

co_rc_t console_widget_NT_t::set_cursor_size(const int curs_size)
{
	co_console_set_cursor_size((void*)own_out_h, curs_size);
	return CO_RC(OK);
}

co_rc_t console_widget_NT_t::op_scroll_up(const co_console_unit& T,
					  const co_console_unit& B,
					  const co_console_unit& L,
					  const co_console_character& C)
{
	SMALL_RECT src_r;
	COORD      dst_c;
	CHAR_INFO  blank;

	src_r.Left   = win_region.Left;
	src_r.Right  = win_region.Right;
	src_r.Top    = T;
	src_r.Bottom = B;

	dst_c.X      = 0;
	dst_c.Y      = src_r.Top - L;

	blank.Attributes     = (C & 0xFF00) >> 8;
	blank.Char.AsciiChar = (C & 0x00FF);

	if (!ScrollConsoleScreenBuffer(own_out_h, &src_r, &src_r, dst_c, &blank))
		co_debug("ScrollConsoleScreenBuffer() error 0x%lx",
			 GetLastError());

	return CO_RC(OK);
}

co_rc_t console_widget_NT_t::op_scroll_down(const co_console_unit& T,
					    const co_console_unit& B,
					    const co_console_unit& L,
					    const co_console_character& C)
{
	SMALL_RECT src_r;
	COORD      dst_c;
	CHAR_INFO  blank;

	src_r.Left   = win_region.Left;
	src_r.Right  = win_region.Right;
	src_r.Top    = T;
	src_r.Bottom = B;

	dst_c.X = 0;
	dst_c.Y = src_r.Top + L;

	blank.Attributes     = (C & 0xFF00) >> 8;
	blank.Char.AsciiChar = (C & 0x00FF);

	if (!ScrollConsoleScreenBuffer(own_out_h, &src_r, &src_r, dst_c, &blank))
		co_debug("ScrollConsoleScreenBuffer() error 0x%lx",
			 GetLastError());

	return CO_RC(OK);
}

co_rc_t console_widget_NT_t::op_bmove(const co_console_unit& Y,
				      const co_console_unit& X,
				      const co_console_unit& T,
				      const co_console_unit& L,
				      const co_console_unit& B,
				      const co_console_unit& R)
{
	SMALL_RECT src_r;
	COORD      dst_c;

	dst_c.Y = Y;
	dst_c.X = X;

	src_r.Top    = T;
	src_r.Left   = L;
	src_r.Bottom = B;
	src_r.Right  = R;

	if(!ScrollConsoleScreenBuffer(own_out_h,
				      &src_r,
				      &win_region,
				      dst_c,
				      &blank))
		co_debug("ScrollConsoleScreenBuffer() error 0x%lx", GetLastError());

	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_putcs(
		const co_console_unit&      Y,
		const co_console_unit&      X,
		const co_console_character* D,
		const co_console_unit&      C)
{
	int count;

	if ((count = C) <= 0)
		return CO_RC(ERROR);

	if(Y >= win_size.Y || X >= win_size.X) {
		co_debug("putc error");
		return CO_RC(ERROR);
	}

	SMALL_RECT r;
	COORD      c;

	c.X    = \
	r.Left = X;

	c.Y      = \
	r.Top    = \
	r.Bottom = Y;

	if (--count + r.Left > console->config.x)
		count = console->config.x - r.Left;
	r.Right = r.Left + count;

	co_console_cell_t* cells = (co_console_cell_t*)D;
	CHAR_INFO*         ci    = &screen[(win_size.X * r.Top) + r.Left];

	do {
		ci->Attributes	       = cells->attr;
		(ci++)->Char.AsciiChar = (cells++)->ch;
	} while (count--);

	if (!WriteConsoleOutput(own_out_h, screen, win_size, c, &r))
		co_debug("WriteConsoleOutput() error 0x%lx", GetLastError());

	return CO_RC(OK);
}

co_rc_t
console_widget_NT_t::op_putc(
		const co_console_unit&      Y,
		const co_console_unit&      X,
		const co_console_character& C)
{
	SMALL_RECT r;
	COORD      c;

	if (Y >= win_size.Y || X >= win_size.X) {
		co_debug("putc error");
		return CO_RC(ERROR);
	}

	c.X     = \
	r.Left  = \
	r.Right = X;

	c.Y      = \
	r.Top    = \
	r.Bottom = Y;

	CHAR_INFO* ci = &screen[(win_size.X * r.Top) + r.Left];

	ci->Attributes     = (C & 0xFF00) >> 8;
	ci->Char.AsciiChar = (C & 0x00FF);

	if (!WriteConsoleOutput(own_out_h, screen, win_size, c, &r))
		co_debug("WriteConsoleOutput() error 0x%lx", GetLastError());
	return CO_RC(OK);
}

co_rc_t console_widget_NT_t::op_cursor_pos(const co_cursor_pos_t& P)
{
	COORD c;

	c.X = P.x;
	c.Y = P.y;
	SetConsoleCursorPosition(own_out_h, c);
	return CO_RC(OK);
}


co_rc_t console_widget_NT_t::op_clear(const co_console_unit&	 T,
				      const co_console_unit&	 L,
				      const co_console_unit&	 B,
				      const co_console_unit&	 R,
				      const co_console_character charattr)
{
	SMALL_RECT r;
	CHAR_INFO* s;
	CHAR_INFO  clear_blank;
	COORD      c;
	long       x;
	long       y;

	if (B < T || R < L)
		return CO_RC(ERROR);
	if (T >= win_size.Y || L >= win_size.X)
		return CO_RC(ERROR);
	if (B >= win_size.Y || R >= win_size.X)
		return CO_RC(ERROR);

	clear_blank.Attributes     = charattr >> 8;
	clear_blank.Char.AsciiChar = charattr & 0xff;

	y = T;
	while(y <= B) {
		x = L;
		s = &screen[(win_size.X * y) + x];
		while(x++ <= R)
			*(s++) = clear_blank;
		y++;
	}

	r.Top  = \
	c.Y    = T;

	r.Left = \
	c.X    = L;

	r.Bottom = B;
	r.Right  = R;

	if(!WriteConsoleOutput(own_out_h, screen, win_size, c, &r))
		co_debug("WriteConsoleOutput() error 0x%lx", GetLastError());

	return CO_RC(OK);
}

// TODO
co_rc_t console_widget_NT_t::op_invert(const co_console_unit& Y,
				       const co_console_unit& X,
				       const co_console_unit& C)
{
	return CO_RC(OK);
}

co_rc_t console_widget_NT_t::loop()
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

co_rc_t console_widget_NT_t::title(const char* T)
{
	SetConsoleTitle(T);
	return CO_RC(OK);
}

co_rc_t console_widget_NT_t::idle()
{
	return CO_RC(OK);
}

void send_key(DWORD code)
{
	co_scan_code_t sc;

	sc.mode = CO_KBD_SCANCODE_RAW;
	/* send e0 if extended key */
	if ( code & 0xE000 )
	{
		sc.code = 0xE0;
		co_user_console_handle_scancode( sc );
	}
	sc.code = code & 0xFF;
	co_user_console_handle_scancode( sc );
}

/*
 * First attempt to make the console copy/paste text.
 * This needs more work to get right, but at least it's a start ;)
 */
static int PasteClipboardIntoColinux()
{
	// Lock clipboard for inspection -- TODO: try again on failure
	if ( ! ::OpenClipboard(NULL) )
	{
		co_debug( "OpenClipboard() error 0x%lx !", ::GetLastError() );
		return -1;
	}
	HANDLE h = ::GetClipboardData( CF_TEXT );
	if ( h == NULL )
	{
		::CloseClipboard( );
		return 0;	// Empty (for text)
	}

	unsigned char* s = (unsigned char*) ::GlobalLock(h);

	if ( s == NULL )
	{
		::CloseClipboard( );
		return 0;
	}
	/* Fake keyboard input */
	for ( ; *s != '\0'; ++s )
	{
		co_scan_code_t sc;

		if ( *s == '\n' )
			continue;	// ignore '\n'

		sc.mode = CO_KBD_SCANCODE_ASCII;
		sc.code = *s;
		co_user_console_handle_scancode( sc );
	}
	::GlobalUnlock( h );
	::CloseClipboard( );
	return 0;
}

#define CO_WIN_KEY	255

void console_widget_NT_t::process_key_event(KEY_EVENT_RECORD& ker)
{
	const BYTE vkey     = static_cast<BYTE>(ker.wVirtualKeyCode);
	const WORD flags    = ker.dwControlKeyState;
	const bool released = (ker.bKeyDown == FALSE);
	const bool extended = flags & ENHANCED_KEY;
	WORD       code     = ker.wVirtualScanCode;

	/* Special key processing */
	switch(vkey)
	{
	case VK_LWIN:
	case VK_RWIN: {

		// Check if LeftAlt+Win (detach from colinux)
		if (!released && (flags & LEFT_ALT_PRESSED)) {

			// Release LeftALT before exit
			send_key(0x38 | 0x80);

			window->detach();
			return;
		}

		// Signal Win key pressed/released
		if (released)
			vkey_state[CO_WIN_KEY] &= ~1;
		else
			vkey_state[CO_WIN_KEY] |=  1;

		break;
	}

	case VK_DELETE: {
		if (!released  &&
		    ((flags & (RIGHT_ALT_PRESSED  | LEFT_ALT_PRESSED))  &&
		     (flags & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) &&
		     (flags & (SHIFT_PRESSED))))
		{
			window->send_ctrl_alt_del();
			return;
		}

		break;
	}

	case VK_APPS:		// Window Context Menu
		return;		// Let windows process this keys

	case VK_MENU: // Left ALT pressed
		if (!released) {
			/* Check if AltGr (received as LeftControl+RightAlt) */
			if (extended && (vkey_state[VK_CONTROL] == 0x9D)) {
		    		/* Release Control key */
				send_key(0x9D);
				vkey_state[VK_CONTROL] = 0;
			}
			// Check if Win+LeftAlt (detach from colinux)
			if (vkey_state[CO_WIN_KEY] & 1) {
				// Release WinKey before exit
				send_key(0xE05B | 0x80);
				window->detach();
				return;
			}
		}
		break;

	case 'V':
		if(!released && (vkey_state[CO_WIN_KEY] & 1))
		{
			PasteClipboardIntoColinux( );
			return;
		}
		break;

	default:
		// Handle sticky mode keys after Alt+Space combination
		if (!released) {
			// Release hanging ALT
			if (!(flags & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) &&
			    vkey_state[VK_MENU]) {
				send_key(vkey_state[VK_MENU]);
				vkey_state[VK_MENU] = 0;
			}
			// Release hanging SHIFT
			if (!(flags & SHIFT_PRESSED) && vkey_state[VK_SHIFT]) {
				send_key(vkey_state[VK_SHIFT]);
				vkey_state[VK_SHIFT] = 0;
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
void console_widget_NT_t::process_focus_event(FOCUS_EVENT_RECORD& fer)
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
