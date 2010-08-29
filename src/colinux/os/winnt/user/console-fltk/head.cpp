/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx>, 2005 (c)
 * Dan Aloni <da-x@gmx.net>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 
#include <colinux/user/console-fltk/input.h>
#include <FL/x.H>
#include <colinux/user/console-fltk/main.h>


#define SOFTWARE_COLINUX_CONSOLE_FONT_KEY "Software\\coLinux\\console\\Font"

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

/* copy the str to the clipboard */
int CopyLinuxIntoClipboard(int str_size, const char* str)
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
        //console_widget_t* my_widget = co_user_console_get_window()->get_widget();

        /* allocate memory for clipboard, plus 1 byte for null termination */
        HGLOBAL hMemClipboard = ::GlobalAlloc(GMEM_MOVEABLE,
                (str_size+1));
        if (hMemClipboard==NULL)
        {
                ::CloseClipboard();
                co_debug( "GlobalAlloc() error 0x%lx !", ::GetLastError() );
                return -1;
        }

        /* paste data into clipboard */
        char *p = (char*)::GlobalLock(hMemClipboard);
        strncpy(p, str, str_size);
	*(p+str_size) = 0;
       
        /* unlock memory (but don't free it-- it's now owned by the OS) */
        ::GlobalUnlock(hMemClipboard);
        ::SetClipboardData(CF_TEXT, hMemClipboard);
        /* and we're done */
        ::CloseClipboard();

        return 0;
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

