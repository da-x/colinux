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

#include  "widget.h"
#include "input.h"

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

class client_console_window : public Fl_Window
{
    typedef Fl_Window           super_;
    typedef console_log_window  self_t;
public:
     client_console_window(console_input&, int w, int h, const char* label=0 );
    ~client_console_window( );

    void set_console(co_console_t* _console){
        mWidget_->set_console(_console);
        mWidget_->redraw();
    };
    co_rc_t handle_console_event(co_console_message_t* msg){mWidget_->handle_console_event(msg);};
    console_input& mInput_;
protected:
    console_widget_t *mWidget_;
private:
    int handle( int event );

    // Child controls
    Fl_Text_Display         *   wText_;

};


#endif
