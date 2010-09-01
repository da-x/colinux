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

#include <colinux/user/console-fltk/main.h>
#include <colinux/user/console-fltk/widget.h>

COLINUX_DEFINE_MODULE("colinux-console-fltk");

#define SOFTWARE_COLINUX_CONSOLE_FONT_KEY "Software\\coLinux\\console\\Font"
/*
 * Storage of current keyboard state.
 * For every virtual key (256), it is 0 (for released) or the scancode
 * of the key pressed (with 0xE0 in high byte if extended).
 */
#define VKEY_LEN 256
static WORD vkey_state[VKEY_LEN];
static bool winkey_state;

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
int PasteClipboardIntoColinux(void)
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

/* copy the entire screen to the clipboard, adding new-line chars at the end of each line */
int CopyLinuxIntoClipboard(void)
{
	/* Lock clipboard for writing -- TODO: try again on failure */
	if ( ! ::OpenClipboard(NULL) )
	{
		co_debug("OpenClipboard() error 0x%lx !", ::GetLastError());
		return -1;
	}

	/* clear clipboard */
	if ( ! ::EmptyClipboard() )
	{
		co_debug("EmptyClipboard() error 0x%lx !", ::GetLastError());
		return -1;
	}

	/* get control to the widget */
	console_widget_t* my_widget = co_user_console_get_window()->get_widget();

	/* allocate memory for clipboard, plus 1 byte for null termination */
	HGLOBAL hMemClipboard = ::GlobalAlloc(GMEM_MOVEABLE,
		(my_widget->screen_size_bytes()+1));
	if (hMemClipboard==NULL)
	{
		::CloseClipboard();
		co_debug( "GlobalAlloc() error 0x%lx !", ::GetLastError() );
		return -1;
	}

	/* paste data into clipboard */
	my_widget->copy_mouse_selection((char*)::GlobalLock(hMemClipboard));

	/* unlock memory (but don't free it-- it's now owned by the OS) */
	::GlobalUnlock(hMemClipboard);
	::SetClipboardData(CF_TEXT, hMemClipboard);

	/* and we're done */
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
	const WORD released = flags & KF_UP;
	WORD       code     = flags & 0xFF;

	// Special key processing
	switch ( vkey )
	{
	case VK_LWIN:
	case VK_RWIN:
		// special handling of the Win+V key (paste into colinux)
		winkey_state = released ? false : true;
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
	case VK_END:
		if ( !released && winkey_state )
		{
			PasteClipboardIntoColinux();
			return 1;	/* key processed */
		}
		break;
		
	case 'C':
	case VK_HOME:
		if ( !released && winkey_state )
		{
			CopyLinuxIntoClipboard( );
			return 1;	/* key processed */
		}
		break;

	case VK_PRIOR:
		if ( !released && winkey_state )
		{
			// page up with windows key
			console_widget_t* my_widget = co_user_console_get_window()->get_widget();
			my_widget->scroll_page_up();
			return 1;	/* key processed */
		}
		break;

	case VK_NEXT:
		if ( !released && winkey_state )
		{
			// page down with windows key
			console_widget_t* my_widget = co_user_console_get_window()->get_widget();
			my_widget->scroll_page_down();
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
		// Lost keyboard focus. Release all pressed keys.
		for(int i = 0; i < VKEY_LEN; i++)
			if ( vkey_state[i] )
			{
				handle_scancode( vkey_state[i] );
				vkey_state[i] = 0;
			}
		winkey_state = false;
	}
}

int ReadRegistry(int key)
{
	HKEY hKey;

	DWORD value=-1, reg_size = sizeof(value);

	if(::RegOpenKeyEx(HKEY_CURRENT_USER, TEXT(SOFTWARE_COLINUX_CONSOLE_FONT_KEY), 0, KEY_READ, &hKey)==ERROR_SUCCESS)
	{
		switch(key)
		{
			case REGISTRY_FONT_SIZE:
				if(::RegQueryValueEx(hKey, TEXT("Size"), NULL, NULL, (BYTE*)&value, &reg_size)!=ERROR_SUCCESS)
					value = -1;
				break;

			case REGISTRY_FONT:
				if(::RegQueryValueEx(hKey, TEXT("Font"), NULL, NULL, (BYTE*)&value, &reg_size)!=ERROR_SUCCESS)
					value = -1;
				break;

			case REGISTRY_COPYSPACES:
				if(::RegQueryValueEx(hKey, TEXT("CopySpaces"), NULL, NULL, (BYTE*)&value, &reg_size)!=ERROR_SUCCESS)
					value = -1;
				break;

			case REGISTRY_EXITDETACH:
				if(::RegQueryValueEx(hKey, TEXT("ExitDetach"), NULL, NULL, (BYTE*)&value, &reg_size)!=ERROR_SUCCESS)
					value = -1;
				break;

			default:
				break;
		}
	}
	RegCloseKey(hKey);

	return value;
}

int WriteRegistry(int key, int new_value)
{
	HKEY hKey;

	DWORD value=new_value, reg_size = sizeof(value);
	LONG retval;
	retval = ::RegCreateKeyEx(HKEY_CURRENT_USER, TEXT(SOFTWARE_COLINUX_CONSOLE_FONT_KEY), 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if(retval==ERROR_SUCCESS)
	{
		switch(key)
		{
			case REGISTRY_FONT_SIZE:
				::RegSetValueEx(hKey, TEXT("Size"), NULL, REG_DWORD, (BYTE*)&value, reg_size);
				break;

			case REGISTRY_FONT:
				::RegSetValueEx(hKey, TEXT("Font"), NULL, REG_DWORD, (BYTE*)&value, reg_size);
				break;

			case REGISTRY_COPYSPACES:
				::RegSetValueEx(hKey, TEXT("CopySpaces"), NULL, REG_DWORD, (BYTE*)&value, reg_size);
				break;

			case REGISTRY_EXITDETACH:
				::RegSetValueEx(hKey, TEXT("ExitDetach"), NULL, REG_DWORD, (BYTE*)&value, reg_size);
				break;

			default:
				break;
		}
	}
	RegCloseKey(hKey);

	return value;
}

int main(int argc, char *argv[])
{
	// Initialize keyboard hook
	memset(vkey_state, 0, sizeof(vkey_state) );
	current_hook = SetWindowsHookEx(WH_KEYBOARD,
					keyboard_hook,
					NULL,
					GetCurrentThreadId());

	// Run main console procedure
	return co_user_console_main(argc, argv);
}
