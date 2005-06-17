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
#include "select_monitor.h"
#include "console.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>


/**
 * Class constructor.
 */
select_monitor_window::select_monitor_window( int w, int h )
    : super_( w,h, "Select monitor..." )
    , id_map_count_( 0 )
{
    ctl_browser_ = new Fl_Hold_Browser( 0,0, w,h-20 );
    ctl_browser_->when( FL_WHEN_RELEASE_ALWAYS );
    ctl_browser_->callback( on_sel_change, this );

    btn_select_  = new Fl_Return_Button( 0,h-20, w/2,20, "Select" );
    btn_select_->callback( on_button, this );

    btn_cancel_ = new Fl_Button( w/2,h-20, w/2,20, "Cancel" );
    btn_cancel_->callback( on_button, this );

    resizable( ctl_browser_ );

    end( );
}

/**
 * Called after user selects, cancels or close window.
 *
 * Informs the main application we are done and pass the selected ID.
 */
void select_monitor_window::on_button( Fl_Widget* w, void* v )
{
    self_t * this_ = reinterpret_cast<self_t *>( v );

    if ( w == this_->btn_select_ )
    {
        int selection = this_->ctl_browser_->value( );
        if ( selection > 0 && selection <= this_->id_map_count_ )
        {
            // Post to the main thread message queue
            typedef console_main_window::tm_data_t tm_data_t;
            tm_data_t* data
                = new tm_data_t( console_main_window::TMSG_MONITOR_SELECT,
                                this_->id_map_[selection-1] );
            Fl::awake( data );
        }
    }

    // Nothing more to do, destroy window
    delete this_;
}

/**
 * Called after selection changes.
 *
 * Enable/disable "Select" button if no selection.
 */
void select_monitor_window::on_sel_change( Fl_Widget*, void* v )
{
    self_t * this_ = reinterpret_cast<self_t *>( v );
    int selection = this_->ctl_browser_->value( );
    if ( selection > 0 && selection <= this_->id_map_count_ )
        this_->btn_select_->activate( );
    else
        this_->btn_select_->deactivate( );
}

/**
 * Populate selection box and show window.
 *
 * If no instances running, display a warning box and returns false.
 */
bool select_monitor_window::start( )
{
    // Any error message is displayed by load_monitors_list()
    if ( !load_monitors_list() )
        return false;

    // No instances running, quit
    if ( id_map_count_ == 0 )
    {
        Fl::error(  "No coLinux instances running!\n"
                    "Start a new one first." );
        return false;
    }

    // Select first item
    ctl_browser_->value( 1 );

    // Ok, show window and return true to signal success
    show();

    return true;
}

/**
 * Fetch monitor list from the driver.
 *
 * Returns false on error.
 */
bool select_monitor_window::load_monitors_list( )
{
    co_manager_handle_t             handle;
    co_manager_ioctl_monitor_list_t list;
    co_rc_t                         rc;

    memset( id_map_, 0, sizeof(id_map_) );
    id_map_count_ = 0;
    ctl_browser_->clear( );

    handle = co_os_manager_open( );
    if ( handle == NULL )
    {
        Fl::error(  "Failed to open coLinux manager!\n"
                    "Check the driver is installed." );
        return false;
    }

    // :FIXME: Check API version

    rc = co_manager_monitor_list( handle, &list );
    co_os_manager_close( handle );
    if ( !CO_OK(rc) )
    {
        Fl::error(  "Failed to get monitor list!\n"
                    "Check your colinux version." );
        return false;
    }

    assert( list.count <= CO_MAX_MONITORS );

    for ( unsigned i = 0; i < list.count; ++i )
    {
        char buf[32];
        id_map_[i] = list.ids[i];
        snprintf( buf, sizeof(buf), "Monitor %d (pid=%d)\t", i, id_map_[i] );
        ctl_browser_->add( buf );
    }
    id_map_count_ = list.count;

    return true;
}
