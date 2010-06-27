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

/*
 * TODO:
 *      - Discover how to make the text copy/paste 'able.
 *      - Maybe two columns: 1 for source name a the other with the text.
 *      - Make the signaling logic better (check if window fully visible).
 */


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
client_console_window::client_console_window( console_input& in, int w, int h, const char* label )
    : mInput_(in),super_( w,h, label )
{
    //mWidget_      = new console_widget_t(0, MENU_SIZE_PIXELS, swidth, sheight - 120);
    mWidget_      = new console_widget_t(0, 0, 800, 600);
}

/**
 * Class destructor.
 */
console_log_window::~console_log_window( )
{
}

client_console_window::~client_console_window( )
{
}

/**
 * Helper internal function for sending thread messages.
 */
static void awake_message( bool new_message )
{
    typedef console_main_window::tm_data_t tm_data_t;
    tm_data_t * msg = new tm_data_t( console_main_window::TMSG_LOG_WINDOW );
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
int client_console_window::handle( int event )
{
        int x, y;

        x = Fl::event_x();
        y = Fl::event_y();

    switch(event){
    case FL_FOCUS:
        //if ( is_attached() )
            mInput_.resume();
        break;
    case FL_UNFOCUS:
        //if ( is_attached() )
        //{
            mInput_.reset( false );
            mInput_.pause( );
        //}
        break;
    case FL_DRAG:
                mWidget_->mouse_drag(x, y);
                break;
    case FL_RELEASE:
                mWidget_->mouse_release(x, y);
                break;
    case FL_MOUSEWHEEL:
                mWidget_->scroll_back_buffer(Fl::event_dy());
                break;
    case FL_KEYUP:
    case FL_KEYDOWN:
        //if ( mouse_mode_ != MouseMark )
            return mInput_.handle_key_event( );
        // Any key will stop the mark mode
        //end_mark_mode( );
        return 1;
    case FL_PASTE:
      {
        int c = mInput_.paste_text( Fl::event_length(), Fl::event_text() );
        //status( "%ddd of %d characters pasted...\n",c, Fl::event_length() );
        return 1;
      }
    }
    
    // Let base class handle other events
    return super_::handle( event );
}

