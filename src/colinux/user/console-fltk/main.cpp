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
#include "main.h"
#include "console.h"

extern "C" {
    #include <colinux/common/version.h>
    #include <colinux/user/debug.h>
}

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FL/Fl.H>

// Needed for co_debug*() routines (one for each application)
//COLINUX_DEFINE_MODULE( "colinux-console-fltk" )


/**
 * "Message of the day" strings to display after startup.
 */
static char MOTD_HELP[]
    =   "CoLinux " COLINUX_VERSION " (build date: " __DATE__ ")\n"
        "Command line arguments supported:\n"
        "  --help               Show this text\n"
        "  --instance-id <id>   Attach to instance ID\n"
    ;

static console_main_window* global_window = NULL;

void co_user_console_handle_scancode(co_scan_code_t sc)
{
	if (!global_window)
		return;
	
	global_window->handle_scancode(sc);
}

/**
 * Parse application command line arguments and fills params structure
 * with the parsing result.
 *
 * Returns zero on success or the number with the position of the parameter
 * that failed to be parsed.
 */
static
int parse_args( int argc, char** args, console_parameters_t& params )
{
    /* Set default parameter values */
    params.instance_id      = CO_INVALID_ID;
    params.motd             = NULL;

    /* Check for no parameters */
    if ( argc == 1 )
        return 0;

    /* Parameter parsing loop */
    for ( int i = 1; i < argc; ++i )
    {
        if ( !strcmp(args[i],"-h") || !strcmp(args[i],"--help") )
        {
            /* Show usage help on the log window after start */
            params.motd = MOTD_HELP;
        }
        else if ( !strcmp(args[i],"-a") || !strcmp(args[i],"--instance-id") )
        {
            if ( i + 1 == argc )
                return i;
            params.instance_id = atoi( args[++i] );
        }
        else if ( Fl::arg(argc,args,i) > 0 )
        {
            // FLTK increments <i> for us, so decrement it
            --i;
        }
        else
        {
            co_debug( "Invalid parameter '%s'...\n", args[i] );
            return i;
        }
    }

    return 0;
}


/**
 * Fltk Console application entry.
 */
int co_user_console_main( int argc, char **argv )
{
    console_parameters_t    params;
    int                     ret;
    char                    err_msg[256];

    // Start debug engine and register cleanup
    co_debug_start( );
    atexit( co_debug_end );

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
    console_main_window * app_window = new console_main_window( );

    // Parse user arguments
    ret = parse_args( argc, argv, params );
    if ( ret )
    {
        co_debug("Error parsing parameter '%s'\n", argv[ret] );
        /* Set an error message as the "motd". */
        snprintf( err_msg, sizeof(err_msg), "Error parsing parameter %d: '%s'!\n", ret, argv[ret] );
        err_msg[sizeof(err_msg)-1] = '\0'; // ensure it's null terminated
        params.motd = err_msg;
    }

    // Setup main window
    app_window->start( params );

    /*
     * We need to call show(argc,argv) or we will end with a 'non-native'
     * color-scheme (at least on Windows). As the FLTK parameters were
     * already processed in parse_args(), we just fake no-args here.
     *
     * Note that the user can decide to start the application minimized
     * with "-iconic" as parameter.
     */
    char* fake_argv[2] = { "colinux-console-fltk", NULL };
    app_window->show( 1, fake_argv );

    // Enter main window loop
    return Fl::run( );
}