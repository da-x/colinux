/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 
#ifndef __COLINUX_USER_CONSOLE_MAIN_H__
#define __COLINUX_USER_CONSOLE_MAIN_H__

extern "C" {
    #include <colinux/common/common.h>
}


/**
 * Console startup parameters
 *
 * columns & lines need a kernel patch before they can work.
 */
struct console_parameters_t
{
    co_id_t     instance_id;    // colinux instance id to attach to
    const char* font_name;      // Name of the font to use
    int         font_size;      // Size of the font to use
    int         columns;        // Number of columns
    int         lines;          // Number of lines
    char  *     motd;           // Message of the day
};


#endif  // __COLINUX_USER_CONSOLE_MAIN_H__
