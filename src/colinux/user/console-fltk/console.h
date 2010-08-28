/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 
#ifndef __COLINUX_USER_CONSOLE_H__
#define __COLINUX_USER_CONSOLE_H__


#include "input.h"
extern "C" {
    #include <colinux/user/reactor.h>
    #include <colinux/user/monitor.h>
    #include <colinux/user/manager.h>
}
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Double_Window.H>

extern int PasteClipboardIntoColinux();

class console_widget_t;
class console_screen;
class console_log_window;
class console_window_t;
struct console_parameters_t;

/**
 * Main console window.
 */
class console_main_window : public Fl_Double_Window
{
    // Shortcuts to the base types
    typedef Fl_Double_Window        super_;
    typedef console_main_window     self_t;
public:
    void handle_scancode(co_scan_code_t sc);
    // Basic initialization
     console_main_window( );
    ~console_main_window( );

    // Use params for setup and show window
    int start( console_parameters_t &params );

    /**
     * Thread message identifiers for asynch messages.
     */
    enum tm_id
    {
        TMSG_LOG_WINDOW, TMSG_MONITOR_SELECT, TMSG_OPTIONS,
        TMSG_VIEW_RESIZE, TMSG_LAST
    };
    /**
     * Thread messages data structure.
     */
    struct tm_data_t
    {
        tm_id       id;     // Message ID
        unsigned    value;  // 32 bit data value
        void    *   data;   // extra data
        tm_data_t( tm_id i=tm_id(0), unsigned v=0, void* d=0 )
                    : id(i), value(v), data(d) {}
    };

private:
    // FLTK Event handler
    int handle( int event );
    // Handle messages from the colinux instance.
    void handle_message( co_message_t *msg );
    // Write something to the log window
    void log( const char *format, ... );
    // Write text in status bar.
    void status( const char *format, ... );
    // Return true if attached to a monitor
    bool is_attached( ) const          { return monitor_ != NULL; }
    // Attach to the given instance ID
    bool attach( co_id_t pid );
    // Dettach current monitor
    void dettach( );
    // Update menu items & status bar state.
    void update_ui_state( );
    // Resize window to fit console
    void resize_around_console( );
    // Load/Save preferences
    void load_preferences( );
    void save_preferences( );

    // Find first active monitor
    co_id_t find_first_monitor( );
    // Center given window inside this window.
    void center_widget( Fl_Widget *win );
    // Return menu item that has the given callback and user id
    Fl_Menu_Item& get_menu_item( Fl_Callback* handler, int id = 0 );
    // Enable/Disable menu item state, by callback routine
    void set_menu_state( Fl_Callback* handler, bool enabled );

    // Handle mouse events and send it to the colinux instance
    void handle_mouse_event( );
    // Handle the copy from console event
    int handle_mark_event( int event );
    void end_mark_mode( );

    /* Menu handlers */
    static void on_quit( Fl_Widget*, void* );
    static void on_select( Fl_Widget*, void* );
    static void on_attach( Fl_Widget*, void* );
    static void on_dettach( Fl_Widget*, void* );
    static void on_options( Fl_Widget*, void* );
    static void on_power( Fl_Widget*, void* );
    static void on_set_font( Fl_Widget*, void* );
    static void on_inspect( Fl_Widget*, void* );
    static void on_change_view( Fl_Widget*, void* );
    static void on_about( Fl_Widget*, void* );
    static void on_font_select( Fl_Widget*, void* );
    static void on_copy( Fl_Widget*, void* );
    static void on_copy_spaces( Fl_Widget*, void* );
    static void on_scroll_page_up( Fl_Widget*, void* );
    static void on_scroll_page_down( Fl_Widget*, void* );
    static void on_show_hide_log( Fl_Widget*, void* );
    static void on_mark( Fl_Widget*, void* );
    static void on_exit_detach( Fl_Widget*, void* );
    static void on_paste( Fl_Widget*, void* );
    static void on_calibrate( Fl_Widget*, void* );
    static void on_key_seq( Fl_Widget*, void* );
    static void unimplemented( Fl_Widget*, void* );

    // Callback for the reactor API
    static co_rc_t reactor_data( co_reactor_user_t, unsigned char*, unsigned long );
    // Called when idle
    static void on_idle( void* );

private:
    // Colinux Instance Data
    co_id_t                 attached_id_;       // Current attached monitor
    co_reactor_t            reactor_;           // colinux message engine
    co_user_monitor_t   *   monitor_;           // colinux instance monitor
    bool                    fullscreen_mode_;   // true if fullscreen on
    console_input           input_;             // Console input handler

    enum eMouseMode { MouseNormal, MouseMark, MouseCalibrate };
    eMouseMode              mouse_mode_;        // Current mouse mode
    int                     mouse_scale_x_;     // Mouse X axis scaling factor
    int                     mouse_scale_y_;     // Mouse Y axis scaling factor

    Fl_Preferences          prefs_;             // Application preferences

    // Child widgets
    Fl_Menu_Bar         *   menu_;
    Fl_Scroll           *   wScroll_;
    console_screen      *   wScreen_;
    console_widget_t    *   wConsole_;
    console_log_window  *   wLog_;
    Fl_Group            *   wStatus_;
    Fl_Box              *   status_line_;
    Fl_Button           *   btn_log_;

    // Static Data
    static Fl_Menu_Item     menu_items_[];      // Application menu items
    static self_t       *   this_;              // Used by static handlers
};

#endif
