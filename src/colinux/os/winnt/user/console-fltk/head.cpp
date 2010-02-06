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

#include <colinux/user/console-fltk/nmain.h>
#include <colinux/user/console-fltk/input.h>
#include <FL/x.H>

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


static void handle_scancode( WORD code )
{
	co_scan_code_t sc;
	sc.mode = CO_KBD_SCANCODE_RAW;
	/* send e0 if extended key */
	if ( code & 0xE000 )
	{
		sc.code = 0xE0;
		co_user_nconsole_handle_scancode( sc );
	}
	sc.code = code & 0xFF;
	co_user_nconsole_handle_scancode( sc );
}

/*
 * First attempt to make the console copy/paste text.
 * This needs more work to get right, but at least it's a start ;)
 */
static int PasteClipboardIntoColinux( )
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
	unsigned char* s = (unsigned char *) ::GlobalLock( h );
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
		co_user_nconsole_handle_scancode( sc );
	}
	::GlobalUnlock( h );
	::CloseClipboard( );
	return 0;
}


static LRESULT CALLBACK keyboard_hook(
    int nCode,
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

int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR cmdLine, int )
{
	// Initialize keyboard hook
	memset( vkey_state, 0, sizeof(vkey_state) );
	current_hook = SetWindowsHookEx(WH_KEYBOARD,
					keyboard_hook,
					NULL,
					GetCurrentThreadId());

	// "Normalize" arguments
	// NOTE: I choosed to ignore parsing errors here as they should
	//       be caught later
	int argc = 0;
	char **argv = NULL;
	co_os_parse_args( cmdLine, &argc, &argv );

	// Run main console procedure
	return co_user_nconsole_main(argc, argv);
}

/**
 * Called to handle FLTK key events.
 *
 * Returns non-zero if event was handled, zero if not.
 */
int console_input::handle_key_event( )
{
    if ( is_paused() )
        return 0;

    // Get WIN32 event
    MSG msg = fl_msg;
    // MSDN says to only use the low word. Lets play safe...
    msg.message &= 0xFFFF;

    // Make sure it's a keyboard message
    if ( msg.message < WM_KEYFIRST || msg.message > WM_KEYLAST )
        return 0;

    WORD     flags  = msg.lParam >> 16;     // ignore the repeat count
    bool     is_up  = flags & KF_UP;        // key released
    unsigned code   = flags & 0x1FF;        // scancode & bit 16 set if extended

    // Special key processing
    switch ( code )
    {
    case 0x138: // Right Alt
        // If Control pressed, it's AltGr (received as Control+RightAlt)
        if ( !is_up && is_down(0x01D) )
        {
            // Release the Control key (linux knows that AltGr is RightAlt)
            send_scancode( 0x09D );
        }
        break;
    case 0x145: // NumLock
        // Windows says it's an extended key, but in effect is a single byte
        code = 0x45;
        break;
    case 0x45:  // Pause (1st code)
        // Ignore and wait for the second code (0xC5)
        return 1;
    case 0xC5:  // Pause (2nd code)
        // Send special scan code - 0x245 - so handler nows what it is
        send_scancode( 0x245 );
        return 1;
    case 0x54:  // SysRq
        // Windows only sends the released SysRq. Fake it.
        send_scancode( 0x54 );  // Down
        send_scancode( 0xD4 );  // Up
        return 1;
    case 0x137: // PrintScreen
        // Windows only sends the key up event. Fake it.
        send_scancode( 0x12A ); send_scancode( 0x137 ); // E0 2A E0 37
        send_scancode( 0x1AA ); send_scancode( 0x1B7 ); // E0 AA E0 B7
        return 1;
    }

    // Normal key processing
    if ( is_up )
        code |= 0x80;   // setup scan code for key release

    // Send key scancode
    send_scancode( code );

    // Let colinux handle all our keys
    return 1;
}
