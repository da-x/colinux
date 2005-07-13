/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#ifndef __COLINUX_USER_CONSOLE_OPTIONS_H__
#define __COLINUX_USER_CONSOLE_OPTIONS_H__

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Return_Button.H>


/**
 * Console options dialog.
 *
 * This will hold all console options, but for now only the font selection
 * is done.
 */
class console_options_window : public Fl_Window
{
    typedef Fl_Window  super_;
public:
    // Initialize controls
    console_options_window( );
    // Set font to show first
    void select_font( Fl_Font face, int size );

private:
    // Populate controls
    void populate( );
    // Update font sizes list
    void update_font_sizes( Fl_Font face );
    // Font list click handler
    static void on_font_click( Fl_Widget*, void* );
    // Font size list click handler
    static void on_font_size_click( Fl_Widget*, void* );
    // Button click handler
    static void on_button( Fl_Widget*, void* );

private:
    Fl_Font                         face_;
    int                             size_;

    // The tabs control (to be added)
//    Fl_Tabs                     *   tabs_;

    // Font selection tab
    Fl_Group                    *   tabFont_;
    Fl_Hold_Browser             *   wFonts_;
    Fl_Hold_Browser             *   wSizes_;
    Fl_Box                      *   wPreview_;

    // Ok/Cancel buttons
    Fl_Button                   *   bCancel_;
    Fl_Return_Button            *   bOk_;
};

#endif
