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
	#include "../osdep.h"
}

#include <colinux/user/console-fltk/main.h>

COLINUX_DEFINE_MODULE("colinux-console-fltk");


/*
 * Storage of current keyboard state.
 * For every virtual key (256), it is 0 (for released) or the scancode
 * of the key pressed (with 0xE0 in high byte if extended).
 */
static WORD vkey_state[256];

/*
 * Handler of hook for keyboard events
 */
static HHOOK current_hook;


static void handle_scancode(WORD code)
{
	co_scan_code_t sc;
	sc.mode = CO_KBD_SCANCODE_RAW;
	/* send e0 if extended key */
	if ( code & 0xE000 )
	{
		sc.code = 0xE0;
		co_user_console_handle_scancode(sc);
	}
	sc.code = code & 0xFF;
	co_user_console_handle_scancode(sc);
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
	
	HANDLE h = ::GetClipboardData(CF_TEXT);
	
	if (h == NULL )
	{
		::CloseClipboard();
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
	
	::GlobalUnlock(h);
	::CloseClipboard();
	
	return 0;
}


static LRESULT CALLBACK keyboard_hook(
    int    nCode,
    WPARAM wParam,
    LPARAM lParam
)
{
	/* MSDN says we MUST to pass along the hook chain if nCode < 0 */
	if ( nCode < 0 )
		return CallNextHookEx( current_hook, nCode, wParam, lParam );

	const BYTE vkey     = wParam & 0xFF;
	const WORD flags    = lParam >> 16;	/* ignore the repeat count */
	const bool released = flags & KF_UP;
	WORD       code     = flags & 0xFF;

	/* Special key processing */
	switch ( vkey )
	{
	case VK_LWIN:
	case VK_RWIN:
		// special handling of the Win+V key (paste into colinux)
		if ( released )	vkey_state[255] &= ~1;
		else			vkey_state[255] |=  1;
		// let Windows process it, for now
	case VK_APPS:
		return CallNextHookEx(current_hook, nCode, wParam, lParam);
	case VK_MENU:	/* Check if AltGr (received as LeftControl+RightAlt) */
		if ( (flags & KF_EXTENDED) && !released &&
		     (vkey_state[VK_CONTROL] == 0x009D) )
		{	/* Release Control key */
			handle_scancode( 0x009D );
			vkey_state[VK_CONTROL] = 0;
		}
		break;
	case 'V':
		if ( !released && (vkey_state[255] & 1) )
		{
			PasteClipboardIntoColinux( );
			return 1;	/* key processed */
		}
		break;
	}

	/* Normal key processing */
	if ( flags & KF_EXTENDED )
		code |= 0xE000; /* extended scan-code code */
	if ( released )
	{	/* key was released */
		code |= 0x80;
		if ( vkey_state[vkey] == 0 )
			return 1;	/* ignore release of not pressed keys */
		vkey_state[vkey] = 0;
	}
	else
	{	/* Key pressed */
		vkey_state[vkey] = code | 0x80;
	}

	/* Send key scancode */
	handle_scancode( code );

	/* Let colinux handle all our keys */
	return 1;
}

void co_user_console_keyboard_focus_change( unsigned long keyboard_focus )
{
	if ( keyboard_focus == 0 )
	{
		/*
		 * Lost keyboard focus. Release all pressed keys.
		 */
		for ( int i = 0; i < 255; ++i )
			if ( vkey_state[i] )
			{
				handle_scancode( vkey_state[i] );
				vkey_state[i] = 0;
			}
		vkey_state[255] = 0;
	}
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR cmdLine, int)
{
	// Initialize keyboard hook
	memset(vkey_state, 0, sizeof(vkey_state) );
	current_hook = SetWindowsHookEx(WH_KEYBOARD,
					keyboard_hook,
					NULL,
					GetCurrentThreadId());

	// "Normalize" arguments
	// NOTE: I choosed to ignore parsing errors here as they should
	//       be caught later
	int    argc = 0;
	char** argv = NULL;
	
	co_os_parse_args(cmdLine, &argc, &argv);

	// Run main console procedure
	return co_user_console_main(argc, argv);
}
