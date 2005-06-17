/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#include "about.h"
extern "C" {
    #include <colinux/common/version.h>
}
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>


/**
 * Class constructor.
 * Set up window layout.
 */
about_box::about_box( )
    : super_( 300, 300, "About CoLinux Console..." )
{
    Fl_Box * box = new Fl_Box( 20,20, 260,110, "CoLinux v" COLINUX_VERSION );
    box->box( FL_PLASTIC_DOWN_BOX );
    box->color( fl_rgb_color(206,224,250) );    // soft blue
    box->labelfont( FL_TIMES | FL_BOLD | FL_ITALIC );
    box->labelsize( 32 );
    box->labeltype( FL_SHADOW_LABEL );

    // We need this to comply with the FLTK licence, but it can be on the
    // documentation instead of here. I put it here because I didn't know
    // what else to add on this box ;-)
    box = new Fl_Box( 20,150, 260,80 );
    box->box( FL_THIN_DOWN_BOX );
    box->labelsize( 12 );
    box->align( FL_ALIGN_CENTER|FL_ALIGN_INSIDE|FL_ALIGN_WRAP );
    box->label( "This program uses code from the FLTK project.\n"
                "http://www.fltk.org" );

    Fl_Button * btn = new Fl_Button( 20,250, 260,30, "Close" );
    btn->box( FL_PLASTIC_UP_BOX );
    btn->callback( on_close, this );

    end( );
}

/**
 * Called on close button click.
 */
void about_box::on_close( Fl_Widget* w, void* v )
{
    about_box* this_ = reinterpret_cast<about_box*>( v );
    delete this_;
}
