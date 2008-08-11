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
#include <FL/Fl_Pixmap.H>

// colinux_logo_3d_xpm data
#include "img/colinux_logo_3d.xpm"

/**
 * Class constructor.
 * Set up window layout.
 */
about_box::about_box( )
    : super_( 200, 350, "About CoLinux Console..." )
{
    Fl_Pixmap* logo = new Fl_Pixmap( colinux_logo_3d_xpm );

    Fl_Box * box = new Fl_Box( 10,10, 180,180 );
    box->box( FL_PLASTIC_UP_BOX );
    box->color( fl_rgb_color(206,224,230) );    // soft blue
    box->image( logo );

    box = new Fl_Box( 10,200, 180,80 );
    box->box( FL_THIN_DOWN_BOX );
    box->labelsize( 14 );
    box->labelfont( FL_HELVETICA_BOLD_ITALIC );
    box->labeltype( FL_ENGRAVED_LABEL );
    box->align( FL_ALIGN_CENTER|FL_ALIGN_INSIDE|FL_ALIGN_WRAP );
    box->label( "C o L i n u x   v " COLINUX_VERSION "\n"
		__DATE__ " " __TIME__ "\n"
		"\n"
		"http://www.colinux.org\n" );

    Fl_Button * btn = new Fl_Button( 10,310, 180,30, "Close" );
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
    Fl::delete_widget( this_ );
}
