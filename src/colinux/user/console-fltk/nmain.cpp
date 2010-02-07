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

#include "nconsole.h"

#include <FL/Fl.H>

static console_main_window* global_window = NULL;

void co_user_nconsole_handle_scancode(co_scan_code_t sc)
{
     if (!global_window)
             return;
     global_window->handle_scancode(sc);
}


/**
 * Fltk Console application entry.
 */
int co_user_nconsole_main(int argc, char **argv)
{
    co_rc_t		    rc;
    console_main_window     window;
    int                     ret;

    global_window = &window;
    // Start debug engine and register cleanup
    co_debug_start( );
    //atexit( co_debug_end );

    // This needs to be called for Fl::awake() and Fl::thread_message to work.
    // It needs to be called only once in the main thread to initialize
    // threading support in FLTK.
    Fl::lock( );

    // The FLTK manual say to put this before the first show() of any
    // window in the program. This makes sure we can use Xdbe on servers
    // where double buffering does not exist for every visual (I suppose
    // this is ignored on Windows).
    Fl::visual( FL_DOUBLE | FL_RGB );

    // Create main window object
    //console_main_window * app_window = new console_main_window( );
    //global_window = app_window;

    // Parse user arguments
    rc = window.parse_args(argc, argv);
    if (!CO_OK(rc)){
        co_debug("Error parsing parameter '%08x'\n", int(rc) );
        ret = -1;
        goto out;
    }

    // Setup main window
    rc = window.start();
    if (!CO_OK(rc)) {
	co_debug("Error %08x starting console", (int)rc);
	ret = -1;
	goto out;
    }
    /*
     * We need to call show(argc,argv) or we will end with a 'non-native'
     * color-scheme (at least on Windows). As the FLTK parameters were
     * already processed in parse_args(), we just fake no-args here.
     *
     * Note that the user can decide to start the application minimized
     * with "-iconic" as parameter.
     */
    //char* fake_argv[2] = { "colinux-console-fltk", NULL };
    //window.show( 1, fake_argv );

    // Enter main window loop
    ret = Fl::run();
out:
    co_debug_end();

    return ret;
}

