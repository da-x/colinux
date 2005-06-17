/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#include "input.h"
#include <FL/Fl.H>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


/**
 * Class constructor
 *
 * Event handling starts paused, so be sure to call resume().
 */
console_input::console_input( )
    : monitor_( NULL )
{
    // Set last mouse event abs_x to -1 to signal no events were sent
    last_mouse_.abs_x = unsigned(-1);
}

/**
 * In the base class, the destructor does nothing.
 */
console_input::~console_input( )
{
}

/**
 * Pause event handling.
 *
 * This will make handle_key_event() and handle_mouse_event() to return
 * zero until resume()'ed.
 */
void console_input::pause( )
{
    monitor_ = NULL;
}

/*
 * Resumes event handling.
 *
 * As argument is passed the monitor strucuture used to send the events.
 */
void console_input::resume( co_user_monitor_t* monitor )
{
    monitor_ = monitor;
}

/**
 * Returns true if input is paused.
 */
bool console_input::is_paused( ) const
{
    return monitor_ == NULL;
}

/**
 * Reset event state.
 *
 * Input state (like pressed keys) is reset, by sending events if
 * necessary (and if dettached is false).
 */
void console_input::reset( bool dettached )
{
    // Reset keyboard state
    if ( !dettached && pressed_ )
        for ( unsigned sc = 0; sc < sizeof(kstate_); ++sc )
            if ( is_down(sc) )
                send_scan_( sc | 0x80 );  // Release key
    memset( kstate_, 0, sizeof(kstate_) );
    pressed_ = 0;

    // Reset mouse state (only needed if buttons pressed)
    if ( !dettached && last_mouse_.abs_x != unsigned(-1) && last_mouse_.btns )
    {
        // Release mouse buttons
        last_mouse_.btns = 0;
        send_mouse_event( last_mouse_ );
    }
    last_mouse_.abs_x = unsigned(-1);   // Invalidate last mouse data
}

/**
 * Helper function to send the scancode to the attached monitor.
 *
 * Bit 7 signals it is a key release(1) or a key press(0).
 * Bit 8 signals it is an extended key(1) (having a 0xE0 prefix).
 * Bit 9 signals it is a very special key (only Pause for now)
 *
 * If the "compose key" is pressed, until is released we enter a special
 * mode where all keys are sticky.
 */
void console_input::send_scancode( unsigned code )
{
    const unsigned compose_key = 0x57;  // F11

    assert( !is_paused() );

    if ( !pressed_ && code == compose_key )
    {
        // Start "Compose" Mode. Only signal key is pressed, but don't send
        // any scancode.
        press_key( compose_key );
        return;
    }
    else if ( code == (compose_key | 0x80) )
    {
        // Leave "Compose" Mode. If no keys pressed, send the compose key.
        release_key( compose_key );
        if ( pressed_ == 1 )
        {   // No other keys pressed, send the compose key
            send_scan_( compose_key );
            send_scan_( compose_key | 0x80 );
            return;
        }
        // Release all other keys pressed.
        for ( unsigned sc=0; sc < 0x200; ++sc )
            if ( is_down(sc) )
                send_scan_( sc | 0x80 );
        memset( kstate_, 0, sizeof(kstate_) );
        pressed_ = 0;
        return;
    }

    // Ignore key release codes if in "compose" mode
    if ( is_down(compose_key) && (code & 0x80) )
        return;

    // Check if special key (only Pause, for now)
    if ( code & 0x200 )
    {
        if ( code != 0x245 )
            return;   // We don't know any other special key
        // Send the Pause sequence: E1 1D 45, E1 9D C5
        send_scan_( 0xE1 ); send_scan_( 0x1D ); send_scan_( 0x45 );
        send_scan_( 0xE1 ); send_scan_( 0x9D ); send_scan_( 0xC5 );
        // No state is changed by this key
        return;
    }

    // Check if key already released
    // FIXME: X sends repeats as released keys instead of pressed.
    if ( (code & 0x80) && !is_down(code & 0x17F) )
        return;   // Don't send two releases for the same key

    // Send e0 first if extended key
    if ( code & 0x100 )
        send_scan_( 0xE0 );
    send_scan_( code );

    // Update key state
    if ( code & 0x80 )
        release_key( code & 0x17F );
    else
        press_key( code );
}

/**
 * Helper function to actually send the raw code.
 */
void console_input::send_scan_( unsigned scan )
{
    struct
    {
        co_message_t        message;
        co_linux_message_t  msg_linux;
        co_scan_code_t      code;
    } msg;

    msg.message.from     = CO_MODULE_CONSOLE;
    msg.message.to       = CO_MODULE_LINUX;
    msg.message.priority = CO_PRIORITY_DISCARDABLE;
    msg.message.type     = CO_MESSAGE_TYPE_OTHER;
    msg.message.size     = sizeof(msg) - sizeof(msg.message);
    msg.msg_linux.device = CO_DEVICE_KEYBOARD;
    msg.msg_linux.unit   = 0;
    msg.msg_linux.size   = sizeof(msg.code);
    msg.code.down        = 1;
    msg.code.code        = scan & 0xFF;

    co_user_monitor_message_send( monitor_, &msg.message );
}

/**
 * Send a mouse event to the attached monitor.
 *
 * This is called directly by the main application window.
 */
bool console_input::send_mouse_event( co_mouse_data_t& data )
{
    if ( !monitor_ )
        return false;

    struct
    {
        co_message_t		message;
        co_linux_message_t	msg_linux;
        co_mouse_data_t		mouse;
    } msg;

    msg.message.from     = CO_MODULE_CONSOLE;
    msg.message.to       = CO_MODULE_LINUX;
    msg.message.priority = CO_PRIORITY_DISCARDABLE;
    msg.message.type     = CO_MESSAGE_TYPE_OTHER;
    msg.message.size     = sizeof(msg) - sizeof(msg.message);
    msg.msg_linux.device = CO_DEVICE_MOUSE;
    msg.msg_linux.unit   = 0;
    msg.msg_linux.size   = sizeof(msg.mouse);
    msg.mouse            = data;

    co_user_monitor_message_send( monitor_, &msg.message );
    last_mouse_ = data;

    return true;
}
