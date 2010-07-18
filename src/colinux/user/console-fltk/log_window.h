/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#ifndef __COLINUX_USER_CONSOLE_LOG_WINDOW_H__
#define __COLINUX_USER_CONSOLE_LOG_WINDOW_H__

#include <FL/Fl_Window.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Menu_Bar.H>

#include  "widget.h"
#include "input.h"

class FontSelectDialog;

/**
 * Tool window for displaying log messages.
 *
 * As a side effect, a thread mesage is posted if a new message arrived
 * whitout the focus or visible.
 */
class console_log_window : public Fl_Window
{
    typedef Fl_Window           super_;
    typedef console_log_window  self_t;
public:
     console_log_window( int w, int h, const char* label=0 );
    ~console_log_window( );

    // Add text to log.
    void add( const char* msg );

private:
    int handle( int event );

    // Child controls
    Fl_Text_Display         *   wText_;

};

class console_window_t : public Fl_Window
{
    typedef Fl_Window           super_;
    typedef console_log_window  self_t;
public:
     console_window_t(console_input&, int w, int h, const char* label=0 );
    ~console_window_t( );

    void set_console(co_console_t* _console){
        widget->set_console(_console);
        widget->redraw();
    };
    co_rc_t handle_console_event(co_console_message_t* msg){widget->handle_console_event(msg);};
    console_input& mInput_;
        console_widget_t * get_widget();
        void resize_font(void);
        FontSelectDialog* fsd;


protected:
    console_widget_t *widget;
private:
    int handle( int event );
    /* Menu handlers */
    static void on_copy( Fl_Widget*, void* );
    static void on_paste( Fl_Widget*, void* );
    static void console_font_cb( Fl_Widget*, void* );
  

   // Child widgets
    Fl_Menu_Bar         *   menu_;        // Static Data
    static Fl_Menu_Item     menu_items_[];      // Application menu items
    static console_window_t *this_;
};


#endif
