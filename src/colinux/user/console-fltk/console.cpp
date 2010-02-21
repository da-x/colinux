/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 
#include "console.h"
#include "main.h"
#include "about.h"
#include "input.h"
#include "screen.h"
#include "options.h"
#include "log_window.h"
#include "select_monitor.h"

extern "C" {
    #include <colinux/common/version.h>
    #include <colinux/common/messages.h>
}

#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include <assert.h>
#include <stdio.h>


/* ----------------------------------------------------------------------- */
/**
 * To be removed after all actions implemented...
 *
 * Generic "not implemented" menu handler callback.
 */
void console_main_window::unimplemented( Fl_Widget* w, void* )
{
    assert( this_ );
    Fl::warning( "Function not implemented yet!" );
}

/* ----------------------------------------------------------------------- */
/**
 * Window menu items.
 *
 * This is static so it can be auto-magically initialized.
 * The extra spaces in the labels make the menu a bit less ugly. ;-)
 */
Fl_Menu_Item console_main_window::menu_items_[]
    = {
        { "File", 0,0,0, FL_SUBMENU },
            { " Options... ", 0, on_options, 0, FL_MENU_DIVIDER },
            { " Quit "      , 0, on_quit },
            { 0 },
        { "Input", 0,0,0, FL_SUBMENU },
            { " Mark "              , 0, on_mark      },
            { " Paste "             , 0, on_paste     , 0, FL_MENU_DIVIDER },
            { " Calibrate mouse... ", 0, on_calibrate , 0, FL_MENU_DIVIDER},
            { " Special keys "      , 0,0,0, FL_SUBMENU },
                { " Alt+SysRq+R (unRaw) "      , 0, on_key_seq   , (void*) 1 },
                { " Alt+SysRq+K (saK) "        , 0, on_key_seq   , (void*) 2 },
                { " Alt+SysRq+B (reBoot) "     , 0, on_key_seq   , (void*) 3 },
                { " Alt+SysRq+N (Nice) "       , 0, on_key_seq   , (void*) 4 },
                { " Alt+SysRq+S (Sync) "       , 0, on_key_seq   , (void*) 5 },
                { " Alt+SysRq+U (RemoUnt) "    , 0, on_key_seq   , (void*) 6 },
                { " Alt+SysRq+P (dumP) "       , 0, on_key_seq   , (void*) 7 },
                { " Alt+SysRq+T (Tasks) "      , 0, on_key_seq   , (void*) 8 },
                { " Alt+SysRq+M (Mem) "        , 0, on_key_seq   , (void*) 9 },
                { " Alt+SysRq+E (sigtErm) "    , 0, on_key_seq   , (void*)10 },
                { " Alt+SysRq+I (sigkIll) "    , 0, on_key_seq   , (void*)11 },
                { " Alt+SysRq+L (sigkill aLL) ", 0, on_key_seq   , (void*)12 },
                { " Alt+SysRq+H (Help) "       , 0, on_key_seq   , (void*)13, FL_MENU_DIVIDER },
                { " Alt+Tab "                  , 0, on_key_seq   , (void*)20 },
                { " Alt+Shift+Tab "            , 0, on_key_seq   , (void*)21 },
                { " Ctrl+Esc "                 , 0, on_key_seq   , (void*)22 },
                { " PrintScreen "              , 0, on_key_seq   , (void*)23 },
                { " Pause "                    , 0, on_key_seq   , (void*)24 },
                { " Ctrl+Break "               , 0, on_key_seq   , (void*)25 },
                { " Ctrl+Alt+Del "             , 0, on_key_seq   , (void*)26 },
                { 0 },
            { 0 },
        { "Monitor", 0,0,0, FL_SUBMENU },
            { " Select... ", 0, on_select     , 0, FL_MENU_DIVIDER },
            { " Attach "   , 0, on_attach     },
            { " Dettach "  , 0, on_dettach    , 0, FL_MENU_DIVIDER },
            { " Pause "    , 0, unimplemented },
            { " Resume "   , 0, unimplemented , 0, FL_MENU_DIVIDER },
            { " Power off ", 0, on_power      , (void*)CO_LINUX_MESSAGE_POWER_OFF },
            { " Reboot - Ctrl-Alt-Del ",0, on_power, (void*)CO_LINUX_MESSAGE_POWER_ALT_CTRL_DEL },
            { " Shutdown " , 0, on_power      , (void*)CO_LINUX_MESSAGE_POWER_SHUTDOWN },
            { 0 },
        { "Inspect", 0,0,0, FL_SUBMENU },
            { " Manager Status ", 0, on_inspect, (void*)1 },
            { " Manager Info "  , 0, on_inspect, (void*)2 },
            { 0 },
        { "View", 0,0,0, FL_SUBMENU },
            { " Show Log window "   , 0, on_show_hide_log, 0, FL_MENU_TOGGLE },
            { " Show Status Bar "   , 0, on_change_view  , (void*)1, FL_MENU_TOGGLE|FL_MENU_VALUE },
            { " Full Screen "       , 0, on_change_view  , (void*)2, FL_MENU_TOGGLE|FL_MENU_DIVIDER },
            { 0 },
        { "Help", 0,0,0, FL_SUBMENU },
            { " Help... "      , 0, unimplemented, 0, FL_MENU_DIVIDER },
            { " About coLinux ", 0, on_about },
            { 0 },
        { 0 }
    };

/* ----------------------------------------------------------------------- */
/**
 * Static pointer to the instantiated window.
 *
 * Used by the static handlers.
 * This forces this window to never be instantiated twice, but we should be
 * safe in that respect.
 */
console_main_window * console_main_window::this_ = NULL;

/* ----------------------------------------------------------------------- */
/**
 * Class constructor.
 */
console_main_window::console_main_window( )
  : super_( 640,480, "Cooperative Linux console [" COLINUX_VERSION "]" )
  , monitor_( NULL )
  , fullscreen_mode_( false )
  , mouse_mode_( MouseNormal )
  , mouse_scale_x_( CO_MOUSE_MAX_X )
  , mouse_scale_y_( CO_MOUSE_MAX_Y )
  , prefs_( Fl_Preferences::USER, "coLinux.org", "FLTK console" )
{
    // Set pointer to self for static methods
    this_ = this;
    // Set close callback
    callback( on_quit );

    const int w = this->w();    // shortcut to w()
    const int h = this->h();    // shortcut to h()
    const int mh = 30;          // menu height
    const int sh = 24;          // status bar height

    // Setup window menu
    menu_ = new Fl_Menu_Bar( 0,0, w,mh );
    menu_->box( FL_FLAT_BOX );
    menu_->align( FL_ALIGN_CENTER );
    menu_->when( FL_WHEN_RELEASE_ALWAYS );
    // This doesn't make a copy, so we can use the array directly
    menu_->menu( menu_items_ );
    // Gray all unimplemented menu items
    set_menu_state( unimplemented, false );

    // Console window (inside a scroll group)
    wScroll_ = new Fl_Scroll(0,mh, w,h-mh-sh );
    wScroll_->box( FL_THIN_DOWN_FRAME );
    {
        const int bdx = Fl::box_dx( FL_THIN_DOWN_FRAME );
        const int bdy = Fl::box_dx( FL_THIN_DOWN_FRAME );
        wScreen_ = new console_screen( bdx, bdy + mh,
                                       640 + bdx*2, 400 + bdy*2 );
    }
    wScroll_->end( );

    // Status bar
    wStatus_ = new Fl_Group( 0,h-sh, w,sh );
    wStatus_->box( FL_FLAT_BOX );
        status_line_ = new Fl_Box( 0,h-sh, w-80,sh );
        status_line_->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
        btn_log_ = new Fl_Button( w-80,h-sh+1, 80,sh-2, "Messages" );
        btn_log_->clear_visible_focus( );
        btn_log_->box( FL_ENGRAVED_BOX );
        btn_log_->callback( on_show_hide_log );
    wStatus_->resizable( status_line_ );
    wStatus_->end( );

    resizable( wScroll_ );
    end( );

#ifdef _WIN32
    /*
     * Load application icon and bind it to the main window.
     *
     * Maybe this should not be here, but as long as it's done before the
     * first show() it will work and avoids having an extra co_os_* funtion
     * just for this.
     *
     * FIXME: Icon ID should not be hardcoded.
     */
    icon( (char*)LoadIcon(fl_display, MAKEINTRESOURCE(2)) );
#endif

    // Create (hidden) log window (will be shown upon request)
    wLog_ = new console_log_window( 400,300, "Message Log" );
    center_widget( wLog_ );

    // Update menu items state & status bar
    update_ui_state( );

    // Setup idle handler
    Fl::add_idle( on_idle );
}

/* ----------------------------------------------------------------------- */
/**
 * Release any resources not automatically allocated.
 */
console_main_window::~console_main_window( )
{
    // Free allocated resources
    co_reactor_destroy( reactor_ );
}

/* ----------------------------------------------------------------------- */
/**
 * "File/Quit" Menu Handler
 */
void console_main_window::on_quit( Fl_Widget*, void* )
{
    // Ask for confirmation, but only if attached.
    if ( this_->is_attached() )
    {
	int quit = fl_ask(
		"You are currently attached to a colinux instance.\n"
		"The colinux instance will stay running after you exit.\n"
		"Do you really want to quit?" );
	if ( !quit )
	    return;
	this_->dettach( );
    }

    // Exit, saving preferences first
    this_->save_preferences( );
    Fl::delete_widget( this_->wLog_ );
    this_->wLog_ = NULL;
    Fl::delete_widget( this_ );
}

/* ----------------------------------------------------------------------- */
/**
 * Use params for setup and show window.
 *
 * Returns zero on success.
 */
int console_main_window::start( console_parameters_t &params )
{
    // Create reactor object
    co_rc_t rc = co_reactor_create( &reactor_ );
    if ( !CO_OK(rc) )
    {
        Fl::error( "Failed to initialize 'reactor' structure! (rc=%X)", rc );
        return -1;
    }

    // Use last saved preferences
    load_preferences( );

    // Show "Message Of The Day", if given.
    if ( params.motd )
        log( params.motd );

    // Attach to given instance or to the first available
    if ( params.instance_id != CO_INVALID_ID )
        attach( params.instance_id );
    else
        attach( find_first_monitor() );

    /* Ignore errors, as we can attach latter */

    // Make sure the console window starts with the focus
    wScreen_->take_focus( );

    return 0;
}

/* ----------------------------------------------------------------------- */
/**
 * Window main event handler
 */
int console_main_window::handle( int event )
{
    switch ( event )
    {
    case FL_FOCUS:
        if ( is_attached() )
            input_.resume( monitor_ );
        break;
    case FL_UNFOCUS:
        if ( is_attached() )
        {
            input_.reset( false );
            input_.pause( );
        }
        break;
    case FL_ENTER:
    case FL_LEAVE:
        return 1;
    case FL_PUSH:
    case FL_RELEASE:
    case FL_MOVE:
    case FL_DRAG:
    case FL_MOUSEWHEEL:
        // Mark mode enabled?
        if ( mouse_mode_ == MouseMark )
            return handle_mark_event( event );
        // Pass mouse messages to colinux, if attached
        if ( is_attached() && Fl::event_inside(wScroll_) )
        {
            handle_mouse_event( );
            return 1;
        }
        break;
    case FL_KEYUP:
    case FL_KEYDOWN:
        if ( mouse_mode_ != MouseMark )
            return input_.handle_key_event( );
        // Any key will stop the mark mode
        end_mark_mode( );
        return 1;
    case FL_PASTE:
      {
        //int c = input_.paste_text( Fl::event_length(), Fl::event_text() );
	PasteClipboardIntoColinux();
        status( "%d characters pasted...\n", Fl::event_length() );
        return 1;
      }
    }

    return super_::handle( event );
}

/* ----------------------------------------------------------------------- */
/**
 * Fetches mouse event data and sends it to the colinux instance.
 */
void console_main_window::handle_mouse_event( )
{
    co_mouse_data_t md = { 0,0,0,0 };

    // Get x,y relative to the termninal screen
    int x = wScreen_->mouse_x( Fl::event_x() );
    int y = wScreen_->mouse_y( Fl::event_y() );

    /* Transform to the comouse virtual screen size
     *
     * FIXME:
     *    The scale factors need to be calibrated for each screen resolution
     *    and mode. We need to implement a mouse calibrate option.
     */
    md.abs_x = mouse_scale_x_*x / wScreen_->w();
    md.abs_y = mouse_scale_y_*y / wScreen_->h();

    // Get button and mousewheel state
    unsigned state = Fl::event_state();
    if ( state & FL_BUTTON1 )
        md.btns |= 1;
    if ( state & FL_BUTTON2 )
        md.btns |= 2;
    if ( state & FL_BUTTON3 )
        md.btns |= 4;
    if ( Fl::event() == FL_MOUSEWHEEL )
        md.rel_z = Fl::event_dy();

//    status( "Mouse: (%d,%d)--virt-->(%d,%d) btns=%d z=%d\n",
//                    x,y, md.abs_x,md.abs_y, md.btns, md.rel_z );

    // Send mouse move event to colinux instance
    input_.send_mouse_event( md );
}

/* ----------------------------------------------------------------------- */
/**
 * Helper to center given window inside this window.
 */
void console_main_window::center_widget( Fl_Widget* win )
{
    win->position( x()+(w()-win->w())/2, y()+(h()-win->h())/2 );
}

/* ----------------------------------------------------------------------- */
/**
 * "Help/About" Menu Handler
 */
void console_main_window::on_about( Fl_Widget*, void* )
{
    assert( this_ );
    // Show the "About Box" dialog
    about_box * win = new about_box( );
    this_->center_widget( win );
    win->set_modal( );
    win->show( );
}

/* ----------------------------------------------------------------------- */
/**
 * "Monitor/Select..." Menu Handler
 */
void console_main_window::on_select( Fl_Widget*, void* )
{
    assert( this_ );
    // Create the monitor selection dialog, center & show it
    select_monitor_window * win = new select_monitor_window( 400,200 );
    this_->center_widget( win );
    win->set_modal( );
    win->start( );
    // The dialog will "awake" us with the selected ID and destroy itself
}

/* ----------------------------------------------------------------------- */
/**
 * "File/Options" Menu Handler
 *
 * Open options window and act acordingly.
 */
void console_main_window::on_options( Fl_Widget*, void* )
{
    assert( this_ );
#if 0
    // Open & show the options window
    console_options_window* win = new console_options_window;
    this_->center_widget( win );
    win->set_modal( );
    win->show( );
    // The window will "awake" us with the options selected and destroy itself
#endif
    this_->log( "Not yet implemented..." );
}

/* ----------------------------------------------------------------------- */
/**
 * "Monitor/Attach" Menu Handler
 *
 *  Try first to attach to the last attached colinux.
 *  If fails, try to attach to the first available.
 */
void console_main_window::on_attach( Fl_Widget*, void* )
{
    assert( this_ && !this_->is_attached() );
    if ( !this_->attach(this_->attached_id_) )
        if ( !this_->attach(this_->find_first_monitor()) )
            Fl::error( "No colinux instance could be attached!" );
}

/* ----------------------------------------------------------------------- */
/**
 * "Monitor/Dettach" Menu Handler
 *
 * Simply calls dettach().
 */
void console_main_window::on_dettach( Fl_Widget*, void* )
{
    assert( this_ && this_->is_attached() );
    this_->dettach( );
}

/* ----------------------------------------------------------------------- */
/**
 * "Monitor/Halt" & "Monitor/Reboot" & "Monitor/Poweroff"
 *
 * Sends a CO_LINUX_MESSAGE_POWER_* to the runing instance.
 */
void console_main_window::on_power( Fl_Widget*, void* v )
{
    assert( this_ && this_->is_attached() );

    struct
    {
        co_message_t                message;
        co_linux_message_t          linux_msg;
        co_linux_message_power_t    data;
    } msg;

    msg.message.from     = CO_MODULE_DAEMON;
    msg.message.to       = CO_MODULE_LINUX;
    msg.message.priority = CO_PRIORITY_IMPORTANT;
    msg.message.type     = CO_MESSAGE_TYPE_OTHER;
    msg.message.size     = sizeof(msg.linux_msg) + sizeof(msg.data);
    msg.linux_msg.device = CO_DEVICE_POWER;
    msg.linux_msg.unit   = 0;
    msg.linux_msg.size   = sizeof(msg.data);

    //if ( unsigned(v) == 0 )
    //    msg.data.type = CO_LINUX_MESSAGE_POWER_ALT_CTRL_DEL;
    //else
    //    msg.data.type = CO_LINUX_MESSAGE_POWER_SHUTDOWN;
    msg.data.type = static_cast<co_linux_message_power_type_t>((unsigned)(v));
    co_user_monitor_message_send( this_->monitor_, &msg.message );
}

/* ----------------------------------------------------------------------- */
/**
 * Write text in log window.
 */
void console_main_window::log( const char *format, ... )
{
    char buf[1024];
    va_list ap;

    va_start( ap, format );
    vsnprintf( buf, sizeof(buf), format, ap );
    va_end( ap );

    // Check if it wasn't destroyed during close
    if ( wLog_ )
        wLog_->add( buf );
}

/* ----------------------------------------------------------------------- */
/**
 * Write text in status bar.
 */
void console_main_window::status( const char *format, ... )
{
    char buf[512];
    va_list ap;

    va_start( ap, format );
    vsnprintf( buf, sizeof(buf), format, ap );
    buf[sizeof(buf)-1] = '\0';  /* Make sure it's terminated */
    va_end( ap );

    status_line_->copy_label( buf );
}

/* ----------------------------------------------------------------------- */
/**
 * Returns PID of first monitor.
 *
 * If none found, returns CO_INVALID_ID.
 *
 * TODO: Find first monitor not already attached.
 *       It still isn't possible to get the first non-attached monitor, but
 *       now it fails to attach if already attached.
 */
co_id_t console_main_window::find_first_monitor( )
{
    co_manager_handle_t handle;
    co_manager_ioctl_monitor_list_t	list;
    co_rc_t	rc;

    handle = co_os_manager_open( );
    if ( handle == NULL )
        return CO_INVALID_ID;

    rc = co_manager_monitor_list( handle, &list );
    co_os_manager_close( handle );
    if ( !CO_OK(rc) || list.count == 0 )
        return CO_INVALID_ID;

    return list.ids[0];
}

/* ----------------------------------------------------------------------- */
/**
 * Called when a new message is received from the colinux instance.
 */
void console_main_window::handle_message( co_message_t * msg )
{
    // Messages received from other modules (daemons)
    if ( msg->type == CO_MESSAGE_TYPE_STRING )
    {
        co_module_name_t mod_name;
        ((char *)msg->data)[msg->size - 1] = '\0';
        log( "%s: %s\n", co_module_repr(msg->from, &mod_name), msg->data );
        return;
    }

    // Say something about unrecognized messages received
    co_module_name_t mod_name;
    ((char *)msg->data)[msg->size - 1] = '\0';
    log( "%s: Unexpected message received\n",
            co_module_repr(msg->from, &mod_name) );
}

/* ----------------------------------------------------------------------- */
/**
 * Attach to the given instance ID
 */
bool console_main_window::attach( co_id_t id )
{
    co_rc_t                         rc;
    co_module_t                     modules[1] = { CO_MODULE_CONSOLE, };
    co_monitor_ioctl_video_attach_t ioctl_video;
    co_user_monitor_t           *   mon;

    if ( is_attached() )
    {
        log( "ERROR: Already attached to a console!\n" );
        return false;
    }
    if ( id == CO_INVALID_ID )
        return true; // Ignore this special case ID

    rc = co_user_monitor_open( reactor_, reactor_data, id,
                               modules, sizeof(modules)/sizeof(co_module_t),
                               &mon );
    if ( !CO_OK(rc) )
    {
        log( "Monitor%d: Error connecting! (rc=%X)\n", id, rc );
        return false;
    }

    /* Get pointer to shared video buffer */
    rc = co_user_monitor_video_attach( mon, &ioctl_video );
    if ( !CO_OK(rc) )
    {
        log( "Monitor%d: Error attaching video output! (rc=%X)\n", id, rc );
        co_user_monitor_close( mon );
        return false;
    }

    /* Start rendering coLinux screen */
    wScreen_->attach( ioctl_video.video_buffer );

    attached_id_ = id;
    monitor_ = mon;

    resize_around_console( );
    input_.resume( monitor_ );

    update_ui_state( );
    status( "Successfully attached to monitor %d", id );

    return true;
}

/* ----------------------------------------------------------------------- */
/**
 * Dettach the current attached console.
 */
void console_main_window::dettach( )
{
    if ( !is_attached() )
        return;     // nothing to do

    // Stop the input event handling, but first reset it
    input_.reset( false );
    input_.pause( );
    wScreen_->dettach( );
    co_user_monitor_close( monitor_ );
    monitor_ = NULL;

    update_ui_state( );
    status( "Monitor %d dettached", attached_id_ );
}

/* ----------------------------------------------------------------------- */
/**
 * Called when idle to check the connection status and thread messages.
 */
void console_main_window::on_idle( void* )
{
    assert( this_ );

    if ( this_->is_attached() )
    {
        // Check reactor messages and dettach on connection lost
        co_rc_t rc = co_reactor_select( this_->reactor_, 10 );
        if ( !CO_OK(rc))
        {
            this_->input_.reset( true );
            this_->dettach( );
            this_->log( "Connection to the colinux instance was broken.\n" );
        }
    }

    // Check asynchronous thread messages
    void * message = Fl::thread_message( );
    if ( message != NULL )
    {
        tm_data_t* msg = reinterpret_cast<tm_data_t*>( message );
        switch ( msg->id )
        {
        case TMSG_LOG_WINDOW:
            if ( msg->value )
                this_->btn_log_->labelcolor( FL_RED );
            else
                this_->btn_log_->labelcolor( FL_BLACK );
            this_->btn_log_->redraw( );
            break;
        case TMSG_OPTIONS:
            // No more font selection needed
//          this_->set_console_font( Fl_Font(msg->value), int(msg->data) );
            break;
        case TMSG_MONITOR_SELECT:
            assert( msg->value != CO_INVALID_ID );
            if ( this_->is_attached() )
                this_->dettach( );
            this_->attach( msg->value );
            break;
        case TMSG_VIEW_RESIZE:
            this_->wScroll_->redraw();
            if ( !this_->fullscreen_mode_ )
                this_->resize_around_console( );
            break;
        default:
            this_->log( "Unkown thread message!!!\n" );
        }
        delete msg;
    }
}

/* ----------------------------------------------------------------------- */
/**
 * Called by the reactor when a message is received from the attached
 * colinux instance.
 */
co_rc_t console_main_window::reactor_data(
        co_reactor_user_t user, unsigned char *buffer, unsigned long size )
{
    co_message_t *  msg;
    unsigned long   msg_size;
    long            size_left = size;
    long            position = 0;

    // Split data stream into messages
    while ( size_left > 0 )
    {
        msg = reinterpret_cast<co_message_t *>( &buffer[position] );
        msg_size = msg->size + sizeof(*msg);
        size_left -= msg_size;
        if ( size_left >= 0 && this_ )
            this_->handle_message( msg );
        position += msg_size;
    }

    return CO_RC(OK);
}

/* ----------------------------------------------------------------------- */
/**
 * Adjust window size to the console size.
 */
void console_main_window::resize_around_console( )
{
    int fit_x = wScreen_->w() + 2*Fl::box_dx(wScroll_->box());
    int fit_y = wScreen_->h() + 2*Fl::box_dy(wScroll_->box());
    int w_diff = wScroll_->w() - fit_x;
    int h_diff = wScroll_->h() - fit_y;

    if ( h_diff != 0 || w_diff != 0 )
        size( w() - w_diff, h() - h_diff );
}

/* ----------------------------------------------------------------------- */
/**
 * "Inspect/..." menu handler.
 *
 * Mostly for debugging...
 */
void console_main_window::on_inspect( Fl_Widget*, void* v )
{
    if ( !this_ )
        return;

    co_manager_handle_t handle = co_os_manager_open();
    if ( handle == NULL )
    {
        Fl::error(  "Failed to open coLinux manager!\n"
                    "Check the driver is installed." );
        return;
    }

    co_rc_t rc = CO_RC(OK);

    switch ( int(v) )
    {
    case 1: // Manager Status
        {
            co_manager_ioctl_status_t status;
            rc = co_manager_status( handle, &status );
            if ( CO_OK(rc) )
            {
                this_->log( "Manager Status:\n" );
                this_->log( "  State: %d\n", status.state );
                this_->log( "  Monitors: %d\n", status.monitors_count );
                this_->log( "  Host API Version: %d\n",
                            status.periphery_api_version );
                this_->log( "  Kernel API Version: %d\n",
                            status.linux_api_version );
                this_->log( "Done.\n" );
            }
            break;
        }
    case 2: // Manager Info
        {
            co_manager_ioctl_info_t info;
            rc = co_manager_info( handle, &info );
            if ( CO_OK(rc) )
            {
                this_->log( "Manager Info:\n" );
                this_->log( "  Host Memory Usage Limit: %u MB\n",
                            info.hostmem_usage_limit>>20 );
                this_->log( "  Host Memory Used: %u MB\n",
                            info.hostmem_used>>20 );
                this_->log( "Done.\n" );
            }
            break;
        }
    }

    co_os_manager_close( handle );

    if ( !CO_OK(rc) )
    {
        char rc_err[256];
        co_rc_format_error( rc, rc_err, sizeof(rc_err) );
        this_->log( "Error %08X: %s\n", rc, rc_err );
    }
}

/* ----------------------------------------------------------------------- */
/**
 * "View/..." menu handler
 */
void console_main_window::on_change_view( Fl_Widget*, void* data )
{
    assert( this_ );
    static int  fx,fy, fw,fh;

    const int id = int(data);
    Fl_Menu_Item& mi = this_->get_menu_item( on_change_view, id );
    Fl_Widget* wscro = this_->wScroll_;
    Fl_Widget* wstat = this_->wStatus_;
    const int mh = 30;
    const int sh = 24;
    const int w = this_->w();
    const int h = this_->h();

    switch ( id )
    {
    case 1:  // Show/Hide status bar toogle
        if ( mi.value() )
        {   // Show status bar
            assert( !wstat->visible() );
            wscro->resize ( 0, mh    , w, h - sh - mh );
            wstat->resize( 0, h - sh, w, sh );
            this_->init_sizes();
            wstat->show( );
            if ( ! this_->fullscreen_mode_ )
                this_->resize_around_console( );
        }
        else
        {   // Hide status bar
            assert( wstat->visible() );
            wscro->resize ( 0, mh, w, h - mh );
            wstat->resize( 0, h , w, 0 );
            this_->init_sizes();
            wstat->hide( );
            if ( ! this_->fullscreen_mode_ )
                this_->resize_around_console( );
        }
        this_->redraw( );
        break;
    case 2: // Full screen toogle
        if ( mi.value() )
        {   // Turn full screen mode on
            fx = this_->x();    fy = this_->y();
            fw = this_->w();    fh = this_->h();
            this_->fullscreen( );
            this_->fullscreen_mode_ = true;
        }
        else
        {   // Restore to normal windowed mode
            this_->fullscreen_off( fx,fy, fw,fh );
            // Needed because we could have hide the status bar in fullscreen
            this_->resize_around_console( );
            this_->fullscreen_mode_ = false;
        }
        break;
    }
}

/* ----------------------------------------------------------------------- */
/**
 * Handler for show/hide log window.
 *
 * We can distinguish from the menu action to the status button action by the
 * user data value being 1 (menu item) or zero (status button).
 */
void console_main_window::on_show_hide_log( Fl_Widget* w, void* v )
{
    assert( this_ );

    // Show/hide window
    if ( this_->wLog_->visible() )
        this_->wLog_->hide( );
    else
        this_->wLog_->show( );
    // Update UI hints to the user
    this_->update_ui_state( );
}

/* ----------------------------------------------------------------------- */
/**
 * Load preferences using FLTK Fl_Preferences portable way.
 *
 * FIXME: Check if outside screen boundaries.
 * FIXME: I have read some reports FLTK is not multi-monitors ready.
 */
void console_main_window::load_preferences( )
{
    // Load last saved position
    int ox,oy, ow,oh;
    prefs_.get( "x", ox, -1 );
    prefs_.get( "y", oy, -1 );
    prefs_.get( "w", ow, -1 );
    prefs_.get( "h", oh, -1 );
    if ( ox > 0 && oy > 0 && ow > 100 && oh > 100 )
        resize( ox,oy, ow,oh );
}

/* ----------------------------------------------------------------------- */
/**
 * Save preferences using FLTK Fl_Preferences portable way.
 *
 * NOTE: The actual save is made on automatic object destruction.
 *
 */
void console_main_window::save_preferences( )
{
    // Don't save window position if in full screen mode
    if ( ! fullscreen_mode_ )
    {
        prefs_.set( "x", x() );
        prefs_.set( "y", y() );
        prefs_.set( "w", w() );
        prefs_.set( "h", h() );
    }
}

/* ----------------------------------------------------------------------- */
/**
 * Enable/Disable menu item state, by callback routine.
 *
 * There should be a better way to make this. Seems stupid to do a O(N)
 * search for each menu item we want to change the state.
 */
void console_main_window::set_menu_state( Fl_Callback* handler, bool enabled )
{
    const int count = menu_->size();
    for ( int i = 0; i < count; i++ )
    {
        if ( menu_items_[i].callback() == handler )
        {
            if ( enabled )
                menu_items_[i].activate();
            else
                menu_items_[i].deactivate();
        }
    }
}

/* ----------------------------------------------------------------------- */
/**
 * Returns first menu item with the specified callback and user data.
 */
Fl_Menu_Item&
console_main_window::get_menu_item( Fl_Callback* handler, int id )
{
    const int count = menu_->size();
    for ( int i = 0; i < count; i++ )
        if ( menu_items_[i].callback() == handler
             && menu_items_[i].user_data() == (void*)id )
            return menu_items_[i];
    // This was not made to not find anything
    assert( true );
    return menu_items_[count-1];
}

/* ----------------------------------------------------------------------- */
/**
 * Gray/enable out menu items & update status bar
 */
void console_main_window::update_ui_state( )
{
    const bool attached = is_attached();
    // Menu
    set_menu_state( on_attach   , !attached );
    set_menu_state( on_dettach  ,  attached );
    set_menu_state( on_power    ,  attached );
    set_menu_state( on_paste    ,  attached );
    set_menu_state( on_key_seq  ,  attached );
    // "Show Log window" checkbox
    Fl_Menu_Item& mi = get_menu_item( on_show_hide_log );
    if ( wLog_->visible() )
        mi.set();
    else
        mi.clear();
}

/* ----------------------------------------------------------------------- */
/**
 * Paste contents of the Clipboard into colinux.
 */
void console_main_window::on_paste( Fl_Widget*, void* )
{
    assert( this_ );
    // We will receive a FL_PASTE event to complete this
    Fl::paste( *this_, 1 );
}

/* ----------------------------------------------------------------------- */
/*
 * Start "Mark" action, that is copy from console screen buffer.
 */
void console_main_window::on_mark( Fl_Widget*, void* )
{
    assert( this_ && this_->mouse_mode_!= MouseMark);
    // Check the screen can be marked
    if ( !this_->wScreen_->can_mark() )
    {
        this_->status( "Can't mark text in graphics mode..." );
        return;
    }
    // Disable the "Mark" & "Paste" menu items.
    this_->set_menu_state( on_mark, false );
    this_->set_menu_state( on_paste, false );
    this_->status( "Mark mode enabled. Drag the mouse to copy." );
    // Start mark mode
    this_->mouse_mode_ = MouseMark;
}

/* ----------------------------------------------------------------------- */
/**
 * Handle mouse messages events during "mark" mode.
 */
int console_main_window::handle_mark_event( int event )
{
    static int mx, my;

    int ex = Fl::event_x();
    int ey = Fl::event_y();

    if ( ! Fl::event_inside(wScroll_) )
    {
	ex = mx;
	ey = my;
    }

    switch ( event )
    {
    case FL_PUSH:
        mx = ex;
        my = ey;
        break;
    case FL_DRAG:
        wScreen_->set_marked_text( mx,my, ex,ey );
	wScreen_->damage( 1 );
	status( "Mark: %d,%d --> %d,%d", mx,my, ex,ey );
        break;
    case FL_RELEASE:
        wScreen_->set_marked_text( mx,my, ex,ey );
	status( "Mark: %d,%d --> %d,%d", mx,my, ex,ey );
        end_mark_mode( );
        return 1;
    default:
        return 1;
    }
    return 1;
}

/* ----------------------------------------------------------------------- */
/**
 * Stops copy from console (mark) mode.
 */
void console_main_window::end_mark_mode( )
{
    mouse_mode_ = MouseNormal;
    input_.resume( monitor_ );
    set_menu_state( on_mark, true );
    set_menu_state( on_paste, true );

    // Get marked text
    char * buf = NULL;
    unsigned len = wScreen_->get_marked_text( 0,0 );
    if ( len ) {
	buf = new char[len];
	len = wScreen_->get_marked_text( buf, len );
	wScreen_->damage( 1 );
    }

    if  ( len )
        Fl::copy( buf, len, 1 );

    if ( !len )
	status( "No text returned!" );
    else
        status( "%d bytes in the clipboard.", len );

    delete[] buf;
}

/* ----------------------------------------------------------------------- */

void console_main_window::on_key_seq( Fl_Widget*, void* data )
{
    unsigned sysrq = 0;

    if ( !this_->is_attached() )
        return;

    this_->input_.reset( false );

    switch ( int(data) )
    {
    case  1: /*Alt+SysRq+R*/ sysrq = 0x13; break;
    case  2: /*Alt+SysRq+K*/ sysrq = 0x25; break;
    case  3: /*Alt+SysRq+B*/ sysrq = 0x30; break;
    case  4: /*Alt+SysRq+N*/ sysrq = 0x31; break;
    case  5: /*Alt+SysRq+S*/ sysrq = 0x1F; break;
    case  6: /*Alt+SysRq+U*/ sysrq = 0x16; break;
    case  7: /*Alt+SysRq+P*/ sysrq = 0x19; break;
    case  8: /*Alt+SysRq+T*/ sysrq = 0x14; break;
    case  9: /*Alt+SysRq+M*/ sysrq = 0x32; break;
    case 10: /*Alt+SysRq+E*/ sysrq = 0x12; break;
    case 11: /*Alt+SysRq+I*/ sysrq = 0x17; break;
    case 12: /*Alt+SysRq+L*/ sysrq = 0x26; break;
    case 13: /*Alt+SysRq+H*/ sysrq = 0x23; break;
#define SEND(sc) this_->input_.send_raw_scancode(sc)
    case 20:    // Alt+Tab
	SEND(0x38);SEND(0x0F);
	SEND(0x8F);SEND(0xB8); return;
    case 21:    // Alt+Shift+Tab
	SEND(0x38);SEND(0x2A);SEND(0x0F);
	SEND(0x8F);SEND(0xAA);SEND(0xB8); return;
    case 22:    // Ctrl+Esc
        SEND(0x1D);SEND(0x01);
	SEND(0x81);SEND(0x9D); return;
    case 23:    // PrintScreen
	SEND(0xE0);SEND(0x2A);SEND(0xE0);SEND(0x37);
	SEND(0xE0);SEND(0xB7);SEND(0xE0);SEND(0xAA); return;
    case 24:    // Pause
	SEND(0xE1);SEND(0x1D);SEND(0x45);
	SEND(0xE1);SEND(0x9D);SEND(0xC5); return;
    case 25:    // Ctrl+Break
	SEND(0x1D);SEND(0xE1);SEND(0x1D);SEND(0x45);
	SEND(0xE1);SEND(0x9D);SEND(0xC5);SEND(0x9D); return;
    case 26:    // Ctrl+Alt+Del
	SEND(0x1D);SEND(0x38);SEND(0xE0);SEND(0x53);
	SEND(0xE0);SEND(0xD3);SEND(0xB8);SEND(0x9D); return;
    default:
	return;
    }
    // Send Alt+SysReq+<??>
    SEND(0x38);       SEND(0x54); SEND(sysrq);
    SEND(sysrq|0x80); SEND(0xD4); SEND(0xB8);
#undef SEND
}

/* ----------------------------------------------------------------------- */

void console_main_window::on_calibrate( Fl_Widget*, void* )
{
    /* TODO: */
}

/* ----------------------------------------------------------------------- */
void console_main_window::handle_scancode(co_scan_code_t sc)
{
        if ( !is_attached())
                return;

        struct {
                co_message_t            message;
                co_linux_message_t      msg_linux;
                co_scan_code_t          code;
        } message;

        message.message.from = CO_MODULE_CONSOLE;
        message.message.to = CO_MODULE_LINUX;
        message.message.priority = CO_PRIORITY_DISCARDABLE;
        message.message.type = CO_MESSAGE_TYPE_OTHER;
        message.message.size = sizeof(message) - sizeof(message.message);
        message.msg_linux.device = CO_DEVICE_KEYBOARD;
        message.msg_linux.unit = 0;
        message.msg_linux.size = sizeof(message.code);
        message.code = sc;

        co_user_monitor_message_send(monitor_, &message.message);
}
