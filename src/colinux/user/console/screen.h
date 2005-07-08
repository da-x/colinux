/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#ifndef COLINUX_USER_CONSOLE_SCREEN_H
#define COLINUX_USER_CONSOLE_SCREEN_H

#include <FL/Fl_Widget.H>
#include <time.h>


struct co_video_header;


/* ----------------------------------------------------------------------- */
/*
 * Base class for the screen rendering engine.
 *
 * Each driver viewer derives from this
 */
class screen_render
{
public:
    virtual ~screen_render( ) {}
    virtual void reset( ) = 0;
    virtual void draw( int x, int y ) = 0;
    virtual unsigned long id() const = 0;
    virtual int w() const = 0;
    virtual int h() const = 0;
};


/* ----------------------------------------------------------------------- */
/*
 * coLinux virtual screen.
 *
 * This class monitors the shared video buffer for changes and displays it's
 * contents. As the shared video buffer is used by both the cocon and cofb
 * driver, it needs to call the right rendering method, if one or the other
 * are active.
 *
 */
class console_screen : public Fl_Widget
{
    typedef Fl_Widget           super_;
    typedef console_screen      self_t;
public:
     console_screen( int x,int y, int w,int h );
    ~console_screen( );

    void attach( void* video_buffer );
    void dettach( );

    int mouse_x( int mx ) const { return mx - x(); }
    int mouse_y( int my ) const { return my - y(); }

    void set_marked_text( int x1,int y1, int x2,int y2 );
    const char* get_marked_text( ) const;
    void clear_marked_text( );

private:
    // Do the drawing ourselves
    void draw( );
    // Check callback
    static void refresh_callback( void* v );
    // Initialize rendering engine
    bool engine_init( co_video_header* video );
    // Current rendering engine ID
    unsigned long engine_id() const;
    // Lock video buffer memory (returns NULL on error)
    co_video_header* video_lock( );
    // Release lock on video memory
    void video_unlock( co_video_header* header );

private:
    void           *    video_buffer_;   // Shared video buffer
    clock_t             last_redraw_;    // Time of last redraw
    screen_render  *    render_;         // Current screen render engine
};


#endif // COLINUX_USER_CONSOLE_SCREEN_H
