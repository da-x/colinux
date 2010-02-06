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
#ifndef __COLINUX_USER_SELECT_MONITOR_H__
#define __COLINUX_USER_SELECT_MONITOR_H__

extern "C" {
    #include <colinux/common/common.h>
}

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Return_Button.H>


/**
 * Window that allows the user to select a colinux instance to use.
 */
class select_monitor_window : public Fl_Window
{
    typedef Fl_Window               super_;
    typedef select_monitor_window   self_t;
public:
    // Class constructor
    select_monitor_window( int w, int h );

    // Show chooser window, after loading the instances list.
    bool start( );

private:
    static void on_sel_change( Fl_Widget* w, void* v );
    static void on_button( Fl_Widget* w, void* v );
    bool load_monitors_list( );

private:
    co_id_t                 id_map_[CO_MAX_MONITORS];
    int                     id_map_count_;

    Fl_Hold_Browser     *   ctl_browser_;
    Fl_Button           *   btn_cancel_;
    Fl_Return_Button    *   btn_select_;
};

#endif
