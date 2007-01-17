/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#ifndef __COLINUX_USER_ABOUT_BOX_H__
#define __COLINUX_USER_ABOUT_BOX_H__

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

/**
 * Simple About Box
 */
class about_box : public Fl_Window
{
    typedef Fl_Window  super_;
public:
    // Initialize controls
    about_box( );
private:
    // On close button click
    static void on_close( Fl_Widget* w, void* v );
};

#endif
