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

#include "fontselect.h"
console_window_t *console_window_t::this_ = NULL;

/*
 * TODO:
 *      - Discover how to make the text copy/paste 'able.
 *      - Maybe two columns: 1 for source name a the other with the text.
 *      - Make the signaling logic better (check if window fully visible).
 */
Fl_Menu_Item console_window_t::menu_items_[]
    = {
        { "Edit", 0,0,0, FL_SUBMENU },
            { " Copy "              , 0, on_copy      },
            { " Paste "             , 0, on_paste     , 0, FL_MENU_DIVIDER },
            { 0 },
        { 0 },
        { "Config" , 0, NULL, NULL, FL_SUBMENU },
        { "Font..." , 0, console_font_cb, 0,  FL_MENU_DIVIDER },
 /*       { "Copy trailing spaces", 0, console_copyspaces_cb, 0,
                        FL_MENU_TOGGLE | ((reg_copyspaces) ? FL_MENU_VALUE : 0)},
        { "Exit on Detach", 0, console_exitdetach_cb, 0,
                        FL_MENU_TOGGLE |  ((reg_exitdetach) ? FL_MENU_VALUE : 0)},
   */     { 0 },

    };


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
console_window_t::console_window_t( console_input& in, int w, int h, const char* label )
    : mInput_(in),super_( w,h, label )
{
    this_ = this;
    // Setup window menu
    menu_ = new Fl_Menu_Bar( 0,0, w,24 );
    menu_->box( FL_FLAT_BOX );
    menu_->align( FL_ALIGN_CENTER );
    menu_->when( FL_WHEN_RELEASE_ALWAYS );
    // This doesn't make a copy, so we can use the array directly
    menu_->menu( menu_items_ );
    
  //mWidget_      = new console_widget_t(0, MENU_SIZE_PIXELS, swidth, sheight - 120);
    mWidget_      = new console_widget_t(0, 24, 800, 600);
    
	// Default Font is "Terminal" with size 18
	// Sample WinNT environment: set COLINUX_CONSOLE_FONT=Lucida Console:12
	// Change only font size:    set COLINUX_CONSOLE_FONT=:12
	char* env_font = getenv("COLINUX_CONSOLE_FONT");

	if (env_font)
	{
		char* p = strchr (env_font, ':');

		if (p)
		{
			int size = atoi (p+1);
			if (size >= 4 && size <= 24)
			{
				// Set size
				mWidget_->set_font_size(size);
			}
			*p = 0; // End for Fontname
		}

		// Set new font style
		if(strlen(env_font))
		{
			// Remember: set_font need a non stack buffer!
			// Environment is global static.
			Fl::set_font(FL_SCREEN, env_font);

			// Now check font width
			fl_font(FL_SCREEN, 18); // Use default High for test here
			if ((int)fl_width('i') != (int)fl_width('W'))
			{
				Fl::set_font(FL_SCREEN, "Terminal"); // Restore standard font
				//log("%s: is not fixed font. Using 'Terminal'\n", env_font);
			}
		}
	}
	else
	{
		// use registry values and not environment variable
		//mWidget_->set_font_name(reg_font);
		//mWidget_->set_font_size(reg_font_size);
	}
    
    
}

/**
 * Class destructor.
 */
console_log_window::~console_log_window( )
{
}

console_window_t::~console_window_t( )
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
int console_window_t::handle( int event )
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

void console_window_t::on_copy( Fl_Widget*, void* )
{
    //assert( this_ );
    // We will receive a FL_PASTE event to complete this
    //Fl::paste( *this_, 1 );
}
void console_window_t::on_paste( Fl_Widget*, void* )
{
    //assert( this_ );
    // We will receive a FL_PASTE event to complete this
    //Fl::paste( *this_, 1 );
}
void console_window_t::console_font_cb( Fl_Widget*, void* )
{
        this_->fsd = new FontSelectDialog(this_,
                this_->mWidget_->get_font_name(),
                this_->mWidget_->get_font_size());
        this_->fsd->show();

}

console_widget_t * console_window_t::get_widget()
{
    return mWidget_;
}

void console_window_t::resize_font(void)
{
 //tocopy       resized_on_attach = PFALSE;
 //tocopy       global_resize_constraint();
}

