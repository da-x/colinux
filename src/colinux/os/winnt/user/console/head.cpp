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
#include <colinux/user/console/input.h>
#include <FL/x.H>

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
