/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#ifndef __COLINUX_CONSOLE_INPUT_H__
#define __COLINUX_CONSOLE_INPUT_H__

extern "C" {
    #include <colinux/common/common.h>
    #include <colinux/user/monitor.h>
}

/**
 * Console input object.
 *
 * This object will handle FLTK input events and queue raw messages to send
 * to the colinux instance.
 *
 * Instead of creating a virtual class, I decided to just put the
 * implementation of handle_key_event() in the os/current/user/console-fltk/head.cpp
 * as that is the only OS specific function.
 */
class console_input
{
public:
    // Class constructor/Destructor
    console_input( );
    ~console_input( );

    // Pause event handling.
    void pause( );

    // Resumes event handling
    void resume( co_user_monitor_t* monitor );
    void resume(){monitor_=old_monitor_;};

    // Returns true if input is paused.
    bool is_paused( ) const;

    // Send a mouse event to the attached monitor.
    bool send_mouse_event( co_mouse_data_t& data );

    /**
     * Called by the main window to let FLTK key events to be handled here.
     *
     * Returns non-zero if event was handled, zero if not.
     * This is the only function implemented differentely by each OS.
     */
    int handle_key_event( );

    /**
     * Reset event state.
     *
     * Input state (like pressed keys) is reset, by sending events if
     * necessary (and if dettached is false).
     */
    void reset( bool dettached );

    /**
     * Paste text buffer as input.
     */
    int paste_text( int len, const char* buf );

    /**
     * Send raw scancode directly to colinux.
     */
    void send_raw_scancode( unsigned scancode )     { send_scan_(scancode); }

private:
    // Helper function for sending the key scancode.
    void send_scancode( unsigned scancode );
    // Helper function for actually sending the raw codes
    void send_scan_( unsigned scancode );

    /**
     * Helper functions to manipulate the state array.
     */
    bool is_down( unsigned sc ) const   { return kstate_[sc>>3] & (1<<(sc&7)); }
    void release_key( unsigned sc )     { kstate_[sc>>3] &= ~(1 << (sc & 7)); --pressed_; }
    void press_key( unsigned sc )       { kstate_[sc>>3] |= 1 << (sc & 7); ++pressed_; }

    unsigned char           kstate_[512>>3];        // Keyboard state
    unsigned                pressed_;               // Number of keys pressed
    co_user_monitor_t   *   monitor_;               // The attached monitor
    co_user_monitor_t   *   old_monitor_;               // The attached monitor
    co_mouse_data_t         last_mouse_;            // Last mouse data sent
};


#endif // __COLINUX_CONSOLE_INPUT_H__
