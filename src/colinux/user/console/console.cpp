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
#include "widget.h"
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

/**
 * Window menu items.
 *
 * This is static so we can use it directly.
 * The extra spaces in the labels make the menu a bit less ugly. ;-)
 */
Fl_Menu_Item console_main_window::menu_items_[]
    = {
        { "File", 0,0,0, FL_SUBMENU },
            { " Options... ", 0, on_options, 0, FL_MENU_DIVIDER },
            { " Quit "      , 0, on_quit },
            { 0 },
        { "Edit", 0,0,0, FL_SUBMENU },
            { " Mark " , 0, on_mark },
            { " Paste ", 0, unimplemented },
            { 0 },
        { "Monitor", 0,0,0, FL_SUBMENU },
            { " Select... "  , 0, on_select     , 0, FL_MENU_DIVIDER },
            { " Attach "     , 0, on_attach     },
            { " Dettach "    , 0, on_dettach    , 0, FL_MENU_DIVIDER },
            { " Pause "      , 0, unimplemented },
            { " Resume "     , 0, unimplemented , 0, FL_MENU_DIVIDER },
            { " Ctl-Alt-Del ", 0, on_power      , (void*)0 },
            { " Shutdown "   , 0, on_power      , (void*)1 },
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

/**
 * Static pointer to the instantiated window.
 *
 * Used by the static handlers.
 */
console_main_window * console_main_window::this_ = NULL;

/**
 * Class constructor.
 */
console_main_window::console_main_window( )
  : super_( 640,480, "Cooperative Linux console [" COLINUX_VERSION "]" )
  , monitor_( NULL )
  , prefs_( Fl_Preferences::USER, "coLinux.org", "FLTK console" )
  , fullscreen_mode_( false )
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

    // Console window
    wConsole_ = new console_widget( 0,mh, w,h-mh-sh );

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

    resizable( wConsole_ );
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

/**
 * Release any resources not automatically allocated.
 */
console_main_window::~console_main_window( )
{
    // Free allocated resources
    co_reactor_destroy( reactor_ );
}

/**
 * "File/Quit" Menu Handler
 */
void console_main_window::on_quit( Fl_Widget*, void* )
{
    int quit = 0;

    // Ask for confirmation, but show different message if attached.
    if ( this_->is_attached() )
    {
        quit = fl_ask(  "You are currently attached to a colinux instance.\n"
                        "The colinux instance will stay running after you exit.\n"
                        "Do you really want to quit?" );
        if ( quit )
            this_->dettach( );
    }
    else
        quit = fl_ask( "Do you really want to quit?" );

    // Exit, saving preferences first
    if ( quit )
    {
        this_->save_preferences( );
        delete this_->wLog_;
        this_->wLog_ = NULL;

        /*
         * FLTK v1.1.4 doesn't the have Fl::delete_widget() function, but we
         * need to remember to use that function after upgrading, as that's
         * the right way to do it (it's dangerous to delete a widget inside
         * a callback).
         */
        delete this_;
    }
}

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

    // Load all fonts from system (with any encoding)
    Fl::set_fonts( "*" );

    // Use last saved preferences
    load_preferences( );

    // If font params given, override preferences
    if ( params.font_name || params.font_size )
        set_console_font( params.font_name, params.font_size );

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
    wConsole_->take_focus( );

    return 0;
}

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
        if ( mark_mode_ && Fl::event_inside(wConsole_) )
            return handle_mark_event( event );
        // Pass mouse messages to colinux, if attached
        if ( is_attached() && Fl::event_inside(wConsole_) )
        {
            co_mouse_data_t md;
            // Calculate mouse position relative to console area
            calc_mouse_position( md );
            // Get button and mousewheel state
            unsigned state = Fl::event_state();
            md.btns = 0;
            if ( state & FL_BUTTON1 )
                md.btns |= 1;
            if ( state & FL_BUTTON2 )
                md.btns |= 2;
            if ( state & FL_BUTTON3 )
                md.btns |= 4;
            md.rel_z = Fl::event_dy();
            // Send mouse move event to colinux instance
            input_.send_mouse_event( md );
            return 1;
        }
        break;
    case FL_KEYUP:
    case FL_KEYDOWN:
        if ( !mark_mode_ )
            return input_.handle_key_event( );
        // Any key will stop the mark mode
        end_mark_mode( );
        return 1;
    case FL_PASTE:
        /*
         * FIXME:
         *      I don't see nothing in Fl::event_text() after this.
         *      Need to check what I'm doing wrong...
         *      There is a commented version for getting the windows
         *      clipboard contents in input.c, but we should use a
         *      portable way (after all that's what FLTK should do for us).
         */
        log( "Pasting %d bytes\n"
             "---- BEGIN PASTED TEXT ----\n"
             "%s\n"
             "---- END PASTED TEXT ----\n",
             Fl::event_length(), Fl::event_text() );
        return 1;
    }

    return super_::handle( event );
}

/**
 * Calculate mouse position inside console.
 */
void console_main_window::calc_mouse_position( co_mouse_data_t& md ) const
{
    // Get x,y relative to the true console screen
    int vx = Fl::event_x() - wConsole_->term_x();
    int vy = Fl::event_y() - wConsole_->term_y();
    // Check they are inside
    if ( vx < 0 )
        vx = 0;
    else if ( vx >= wConsole_->term_w() )
        vx = wConsole_->term_w() - 1;
    if ( vy < 0 )
        vy = 0;
    else if ( vy >= wConsole_->term_h() )
        vy = wConsole_->term_h() - 1;
    // Transform to the comouse virtual screen size
    assert( wConsole_->term_w() && wConsole_->term_h() );
    /*
     * FIXME:
     *
     *      This are hardcoded values i saw that worked well enough with gpm.
     *      Need to check why only these values work, and not the 2048 max.
     *      An alternative is to have a calibration option.
     */
    md.abs_x = vx*1600 / wConsole_->term_w();
    md.abs_y = vy*1350 / wConsole_->term_h();
}

/**
 * Center given window inside this window.
 */
void console_main_window::center_widget( Fl_Widget* win )
{
    win->position( x()+(w()-win->w())/2, y()+(h()-win->h())/2 );
}

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

/**
 * "File/Options" Menu Handler
 *
 * Open options window and act acordingly.
 */
void console_main_window::on_options( Fl_Widget*, void* )
{
    assert( this_ );
    // Open & show the options window
    console_options_window* win = new console_options_window;
    const Fl_Font cur_face = this_->wConsole_->get_font_face();
    const int     cur_size = this_->wConsole_->get_font_size();
    this_->center_widget( win );
    win->select_font( cur_face, cur_size );
    win->set_modal( );
    win->show( );
    // The window will "awake" us with the options selected and destroy itself
}

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

    if ( unsigned(v) == 0 )
        msg.data.type = CO_LINUX_MESSAGE_POWER_ALT_CTRL_DEL;
    else
        msg.data.type = CO_LINUX_MESSAGE_POWER_SHUTDOWN;

    co_user_monitor_message_send( this_->monitor_, &msg.message );
}

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

    // We need to check if it wasn't destroyed during close
    if ( wLog_ )
        wLog_->add( buf );
}

/**
 * Write text in status bar.
 */
void console_main_window::status( const char *format, ... )
{
    // We need to hold a static buffer, as FLTK v1.1.4 Fl_Widget seems to not
    // have copy_label() (at least 1.1.6 does)
    static char buf[512];
    va_list ap;

    va_start( ap, format );
    vsnprintf( buf, sizeof(buf), format, ap );
    va_end( ap );

    status_line_->label( buf );
}

/**
 * Returns PID of first monitor.
 *
 * If none found, returns CO_INVALID_ID.
 *
 * TODO: Find first monitor not already attached.
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

/**
 * Called when a new message is received from the colinux instance.
 */
void console_main_window::handle_message( co_message_t * msg )
{
    // Messages from the colinux instance
    if ( msg->from == CO_MODULE_LINUX )
    {
        co_console_message_t * cons_msg;
        cons_msg = reinterpret_cast<co_console_message_t *>( msg->data );
        wConsole_->handle_console_event( cons_msg );
        return;
    }

    // Messages received from other modules (daemons)
    if ( msg->type == CO_MESSAGE_TYPE_STRING )
    {
        co_module_name_t mod_name;
        ((char *)msg->data)[msg->size - 1] = '\0';
        log( "%s: %s", co_module_repr(msg->from, &mod_name), msg->data );
    }
}

/**
 * Attach to the given instance ID
 */
bool console_main_window::attach( co_id_t id )
{
    co_rc_t                         rc;
    co_module_t                     modules[1] = { CO_MODULE_CONSOLE, };
    co_monitor_ioctl_get_console_t  ioctl_con;
    co_user_monitor_t           *   mon;
    co_console_t                *   con;

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

    rc = co_user_monitor_get_console( mon, &ioctl_con );
    if ( !CO_OK(rc) )
    {
        log( "Monitor%d: Error getting console! (rc=%X)\n", id, rc );
        co_user_monitor_close( mon );
        return false;
    }

    rc = co_console_create( ioctl_con.x, ioctl_con.y, 0, &con );
    if ( !CO_OK(rc) )
    {
        log( "Monitor%d: Error creating console! (rc=%X)\n", id, rc );
        co_user_monitor_close( mon );
        return false;
    }

    attached_id_ = id;
    monitor_ = mon;

    wConsole_->attach( con );
    resize_around_console( );
    input_.resume( monitor_ );
    wConsole_->redraw( );

    update_ui_state( );
    status( "Successfully attached to monitor %d", id );

    return true;
}

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
    wConsole_->dettach( );
    co_user_monitor_close( monitor_ );
    monitor_ = NULL;

    update_ui_state( );
    status( "Monitor %d dettached", attached_id_ );
}

/**
 * Called when idle to check the connection status and thread messages.
 */
void console_main_window::on_idle( void* )
{
    assert( this_ );

    if ( !this_->is_dettached() )
    {
        /*
         * With FLTK 1.1.6, on_idle is called a lot, so we need to sleep
         * for a while or the CPU will be always at 100%.
         * A 10 msecs sleep seems to be enough.
         */
        // FIXME: Use a portable sleep [implement co_os_msleep(msecs)]
        ::Sleep( 10 );
    }
    else
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
            this_->set_console_font( Fl_Font(msg->value), int(msg->data) );
            break;
        case TMSG_MONITOR_SELECT:
            assert( msg->value != CO_INVALID_ID );
            if ( this_->is_attached() )
                this_->dettach( );
            this_->attach( msg->value );
            break;
        default:
            this_->log( "Unkown thread message!!!\n" );
        }
        delete msg;
    }
}

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

/**
 * Adjust window size to the console size.
 */
void console_main_window::resize_around_console( )
{
    int fit_x = wConsole_->term_w() + 2*(wConsole_->term_x() - wConsole_->x() );
    int fit_y = wConsole_->term_h() + 2*(wConsole_->term_y() - wConsole_->y() );
    int w_diff = wConsole_->w() - fit_x;
    int h_diff = wConsole_->h() - fit_y;

    if ( h_diff != 0 || w_diff != 0 )
        size( w() - w_diff, h() - h_diff );
}

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

/**
 * "View/..." menu handler
 */
void console_main_window::on_change_view( Fl_Widget*, void* data )
{
    assert( this_ );
    static int  fx,fy, fw,fh;

    const int id = int(data);
    Fl_Menu_Item& mi = this_->get_menu_item( on_change_view, id );
    Fl_Widget* wcon = this_->wConsole_;
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
            wcon->resize ( 0, mh    , w, h - sh - mh );
            wstat->resize( 0, h - sh, w, sh );
            this_->init_sizes();
            wstat->show( );
            if ( ! this_->fullscreen_mode_ )
                this_->resize_around_console( );
        }
        else
        {   // Hide status bar
            assert( wstat->visible() );
            wcon->resize ( 0, mh, w, h - mh );
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

/**
 * Load preferences using FLTK Fl_Preferences portable way.
 *
 * TODO: Check if outside screen boundaries.
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
    // Load last used console font & size
    if ( prefs_.entryExists("font_name") && prefs_.entryExists("font_size") )
    {
        char  name[128];
        int   size;
        prefs_.get( "font_name", name, "", sizeof(name) );
        prefs_.get( "font_size", size, -1 );
        if ( name[0] && size > 0 )
            set_console_font( name, size );
    }
}

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
    // Save console font & size
    Fl_Font face = wConsole_->get_font_face();
    int     size = wConsole_->get_font_size();
    const char* name = Fl::get_font( face );
    if ( name != NULL )
    {
        prefs_.set( "font_name", name );
        prefs_.set( "font_size", size );
    }
}

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
    // "Show Log window" checkbox
    Fl_Menu_Item& mi = get_menu_item( on_show_hide_log );
    if ( wLog_->visible() )
        mi.set();
    else
        mi.clear();
}

/**
 * Selects a new font for the console.
 */
void console_main_window::set_console_font( Fl_Font font, int size )
{
    wConsole_->set_font( font, size );
    resize_around_console( );
    wConsole_->damage( 1 );
}

/**
 * Set console font by face name.
 *
 * Searches all available fonts for the given face name.
 */
void console_main_window::set_console_font( const char* f_face, int f_size )
{
    Fl_Font fi = Fl_Font(0);
    const char* face = Fl::get_font( fi );

    while ( face != NULL )
    {
        if ( strcmp(face,f_face) == 0 )
        {
            set_console_font( fi, f_size );
            return;
        }
        // Next font
        fi = Fl_Font( fi + 1 );
        face = Fl::get_font( fi );
    }
}

/**
 * Paste contents of the Clipboard into colinux.
 */
void console_main_window::on_paste( Fl_Widget*, void* )
{
    assert( this_ );
    // We will receive a FL_PASTE event to complete this
    Fl::paste( *this_ );
}

void console_main_window::on_mark( Fl_Widget*, void* )
{
    assert( this_ && !this_->mark_mode_);
    // Disable the "Mark" & "Paste" menu items.
    this_->set_menu_state( on_mark, false );
    this_->set_menu_state( on_paste, false );
    this_->status( "Mark mode enabled. Drag the mouse to copy." );
    // Start mark mode
    this_->mark_mode_ = true;
}

/**
 * Handle mouse messages events during "mark" mode.
 */
int console_main_window::handle_mark_event( int event )
{
    static int mx, my;

    switch ( event )
    {
    case FL_PUSH:
        mx = Fl::event_x();
        my = Fl::event_y();
        break;
    case FL_DRAG:
        wConsole_->set_marked_text( mx,my, Fl::event_x(),Fl::event_y() );
        break;
    case FL_RELEASE:
        wConsole_->set_marked_text( mx,my, Fl::event_x(),Fl::event_y() );
        end_mark_mode( );
        return 1;
    default:
        return 1;
    }

    // Flush pending drawing operations
    wConsole_->redraw( );

    return 1;
}

/**
 * Stops copy from console (mark) mode.
 */
void console_main_window::end_mark_mode( )
{
    mark_mode_ = false;
    input_.resume( monitor_ );
    set_menu_state( on_mark, true );
    set_menu_state( on_paste, true );

    // Get marked text
    const char* buf = wConsole_->get_marked_text( );

    if  ( !buf )
    {
        status( "Null selection returned!" );
    }
    else
    {
        int len = strlen( buf );
        Fl::copy( buf, len, 1 );
        status( "%d bytes selected.", len );
    }

    wConsole_->clear_marked( );
    wConsole_->redraw( );
}
