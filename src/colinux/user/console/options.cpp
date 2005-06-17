/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#include "options.h"
#include "console.h"

#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * Class constructor.
 */
console_options_window::console_options_window( )
    : super_( 420,370, "Options..." )
    , face_( FL_SCREEN )
    , size_( 18 )
{
    Fl_Box* label;  // temporary label holder

    const int w = this->w();
    const int h = this->h() - 50;

    /* ______________
      |_Console_Font_|_______________________
      | _Font_Face____________  _Font_Size__ |
      ||                      ||            ||
      ||                      ||            ||
      ||                      ||            ||
      ||                      ||            ||
      ||                      ||            ||
      ||                      ||            ||
      ||______________________||____________||
      | _Preview____________________________ |
      ||                                    ||
      ||    Some text as example ...        ||
      ||____________________________________||
      |______________________________________|
    */
    tabFont_ = new Fl_Group( 10,10, w-20,h-20 );

    label = new Fl_Box( 10,10, 100,20, "Font Face" );
    label->align( FL_ALIGN_LEFT|FL_ALIGN_INSIDE );
    tabFont_->add( label );
    wFonts_ = new Fl_Hold_Browser( 10,30, 300,150 );
    wFonts_->when( FL_WHEN_RELEASE_ALWAYS );
    wFonts_->callback( on_font_click, this );
    tabFont_->add( wFonts_ );

    label = new Fl_Box( 320,10, 100,20, "Font Size" );
    label->align( FL_ALIGN_LEFT|FL_ALIGN_INSIDE );
    wSizes_ = new Fl_Hold_Browser( 320,30, 90,150 );
    wSizes_->when( FL_WHEN_RELEASE_ALWAYS );
    wSizes_->callback( on_font_size_click, this );
    tabFont_->add( wSizes_ );

    label = new Fl_Box( 10,190, 100,20, "Preview" );
    label->align( FL_ALIGN_LEFT|FL_ALIGN_INSIDE );
    wPreview_ = new Fl_Box( 10,210, 400,100 );
    wPreview_->align( FL_ALIGN_BOTTOM_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_CLIP );
    const char* preview_text =
          "drwxr-xr-x   3 lucas users 128 Feb 19 20:48 ./\n"
          "drwxr-xr-x   9 root  root  224 May 23 17:14 ../\n"
          "rw-r--r--    1 lucas users 232 Feb 19 03:40 .bash_profile\n"
          "-rw-r--r--   1 lucas users 812 Feb 19 03:40 .bashrc\n"
          "colinux $ echo $COLINUX_ROCKS > /etc/motd";
    wPreview_->label( preview_text );
    wPreview_->box( FL_DOWN_BOX );
    wPreview_->color( FL_BLACK );
    wPreview_->labelcolor( FL_WHITE );

    tabFont_->end( );

    // Ok/Cancel buttons at end
    bCancel_ = new Fl_Button( w-220,h+10, 100,30, "Cancel" );
    bCancel_->callback( on_button, this );
    bOk_  = new Fl_Return_Button( w-110,h+10, 100,30, "Select" );
    bOk_->callback( on_button, this );
    bOk_->deactivate( );    // Start deactivated
    end( );

    // Populate controls
    populate( );
}

/**
 * Set font to show as the current console font.
 */
void console_options_window::select_font( Fl_Font face, int size )
{
    for ( int i = 1; i <= wFonts_->size(); ++i )
        if ( (int)wFonts_->data(i) == int(face) )
        {
            face_ = face;
            size_ = size;
            wFonts_->select( i );
            // Ensure selection is visible
            if ( ! wFonts_->visible(i) )
                wFonts_->middleline( i );
            // Fake user click, so sizes & preview gets updated
            wFonts_->do_callback( );
            return;
        }
}

/**
 * Populate controls with font info.
 */
void console_options_window::populate( )
{
    Fl_Font fi = Fl_Font(0);
    const char* face = Fl::get_font( fi );
    const char* name;
    int attr;

    wFonts_->clear( );
    while ( face != NULL )
    {
        name = Fl::get_font_name( fi, &attr );
        // Ignore bold and/or italic fonts
        if ( attr == 0 )
        {
            fl_font( fi, 18 );
            // Ignore proportional fonts
            if ( fl_width('i') == fl_width('W') )
            {
                char buf[512];
                snprintf( buf, sizeof(buf), "@l@.%d", name );
                wFonts_->add( name, (void*)fi );
            }
        }
        // Next font
        fi = Fl_Font( fi + 1 );
        face = Fl::get_font( fi );
    }
}

/**
 * Update font sizes list browser.
 */
void console_options_window::update_font_sizes( Fl_Font face )
{
    /*
     * FLTK seems to fool us into believing non-scalable fonts can't have
     * different sizes than the ones provided. The truth is that it can say a
     * font only has 9,12,14 sizes and yet it displays 18,20,24 without
     * problem.
     * I decided to force the use of a fixed size list to alleviate this.
     * That means some sizes will show no difference, but there is the
     * preview window to show that.
     */
    int * sizes;
    int len = Fl::get_font_sizes( face, sizes );

    assert( len > 0 );

    // if it is a scalable font, provide a default font size list
    // if ( len >= 1 && sizes[0] == 0 )
    {
        static int default_sizes[]
                = { 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 24 };
        sizes = default_sizes;
        len = sizeof(default_sizes)/sizeof(default_sizes[0]);
    }

    // Update font sizes list box
    wSizes_->clear( );
    for ( int i = 0; i < len; ++i )
    {
        char buf[64];
        snprintf( buf, sizeof(buf), "@r@.%d", sizes[i] );
        buf[sizeof(buf)-1] = '\0';
        wSizes_->add( buf, (void*) sizes[i] );
    }

    // Find nearest match for current size and select it
    for ( int i = 1; i <= len; ++i )
        if ( (int)wSizes_->data(i) >= size_ )
        {
            wSizes_->select( i );
            // Ensure selection is visible
            if ( !wSizes_->visible(i) )
                wSizes_->middleline( i );
            // Fake user click, so preview gets updated
            wSizes_->do_callback( );
            return;
        }

    // If still no match, select the nearest
    int min = (int)wSizes_->data( 1 );
    int max = (int)wSizes_->data( len );
    int i = abs(size_-min) < abs(size_-max)? 1 : len;
    wSizes_->select( i );
    // Ensure selection is visible
    if ( !wSizes_->visible(i) )
        wSizes_->middleline( i );
    // Fake user click, so preview gets updated
    wSizes_->do_callback( );
}

/**
 * Button click handler.
 */
void console_options_window::on_button( Fl_Widget* w, void* v )
{
    console_options_window* this_ = (console_options_window *) v;

    // Set main window console font, if not canceled
    if ( w == this_->bOk_ )
    {
        // Post to the main thread message queue
        typedef console_main_window::tm_data_t tm_data_t;
        tm_data_t* data = new tm_data_t( console_main_window::TMSG_OPTIONS );
        data->value = this_->face_;
        data->data  = (void*)this_->size_;
        Fl::awake( data );
    }

    // All done
    delete this_;
}

/**
 * Called when user clicks a font from the list
 */
void console_options_window::on_font_click( Fl_Widget* w, void* v )
{
    Fl_Hold_Browser *        b = (Fl_Hold_Browser*) w;
    console_options_window * this_ = (console_options_window*) v;

    int selected = b->value();
    if ( selected == 0 )
    {
        // Deactivate OK button if nothing selected
        this_->bOk_->deactivate( );
        return;
    }

    // Update font size list box
    Fl_Font face = Fl_Font( int(b->data(selected)) );
    this_->update_font_sizes( face );
}

/**
 * Called when user clicks a font size from the list
 * Updates preview window.
 */
void console_options_window::on_font_size_click( Fl_Widget* w, void* v )
{
    console_options_window * this_ = (console_options_window*) v;
    Fl_Hold_Browser *        fonts = this_->wFonts_;
    Fl_Hold_Browser *        sizes = this_->wSizes_;

    // Check if they are both selected
    const int fsel = fonts->value();
    const int ssel = sizes->value();
    if ( fsel == 0 || ssel == 0 )
    {
        // Deactivate OK button if no selection
        this_->bOk_->deactivate( );
        return;
    }

    // Get font and size and set preview label font
    this_->face_ = Fl_Font( int(fonts->data(fsel)) );
    this_->size_ = (int) sizes->data( ssel );
    this_->wPreview_->labelfont( this_->face_ );
    this_->wPreview_->labelsize( this_->size_ );
    this_->wPreview_->redraw( );
    this_->bOk_->activate( );
}
