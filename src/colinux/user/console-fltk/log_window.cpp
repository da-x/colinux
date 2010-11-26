/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#include "log_window.h"
#include "console.h"
#include <FL/Fl.H>
#include <assert.h>
#include <string.h>

/**
 * Class constructor.
 */
console_log_window::console_log_window( int w, int h, const char* label )
    : super_( w,h, label )
{
    wText_ = new Fl_Text_Display( 0,0, w,h );
    wText_->buffer( new Fl_Text_Buffer() );
    wText_->insert_position( 0 );

    resizable( wText_ );
    end();
}

/**
 * Class destructor.
 */
console_log_window::~console_log_window( )
{
}

/**
 * Helper internal function for sending thread messages.
 */
static void awake_message( bool new_message )
{
    typedef console_window_t::tm_data_t tm_data_t;
    tm_data_t * msg = new tm_data_t( console_window_t::TMSG_LOG_WINDOW );
    msg->value = new_message;
    Fl::awake( msg );
}

/**
 * Add text at end of buffer.
 *
 * Note: No '\n' is appended, as it can be a partial output.
 */
void console_log_window::add( const char* msg )
{
    wText_->insert_position( wText_->buffer()->length() );
    wText_->show_insert_position( );
    wText_->insert( msg );
    // Send "new message" signal to console
    if ( !visible() )
        awake_message( true );
}

/**
 * FLTK event handler.
 *
 * Just used to signal message read when focused.
 */
int console_log_window::handle( int event )
{
    if ( event == FL_FOCUS )
        awake_message( false );
    // Let base class handle other events
    return super_::handle( event );
}

