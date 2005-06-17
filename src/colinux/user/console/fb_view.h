/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#ifndef COLINUX_USER_CONSOLE_FB_VIEW_H
#define COLINUX_USER_CONSOLE_FB_VIEW_H

extern "C" {
    #include <colinux/common/console.h>
}
#include <FL/Fl_Widget.H>
#include <time.h>


/**
 * Virtual coLinux "framebuffer like" console emulation.
 *
 * Emulates a VGA console screen. By passing the appropriate parameters to
 * the colinux kernel at boot, it can have a different size.
 * The default mode is 640x400, with the default 8x16 font, resulting in the
 * standard 80x25 linux terminal.
 *
 * Instead of relying in the kernel to do the scrollback, it's implemented
 * here by having extra space at the top (sort of software scrollback).
 *
 */
class console_fb_view : public Fl_Widget
{
    typedef Fl_Widget           super_;
    typedef console_fb_view     self_t;
    typedef unsigned short      cell_t;
    typedef unsigned long       pixel_t;
public:
     console_fb_view( int x,int y, int w,int h );
    ~console_fb_view( );

    void attach( co_console_t *console );
    void handle_console_event( co_console_message_t *message );
    void dettach( );

    int mouse_x( int mx ) const { return mx - x(); }
    int mouse_y( int my ) const { return my - y(); }

    void set_marked_text( int x1,int y1, int x2,int y2 );
    const char* get_marked_text( ) const;
    void clear_marked_text( );

protected:
    //FIXME: this is here just to catch external uses of the redraw() method
    void redraw( ) { super_::redraw(); }

    int c2x( int col ) const            { return col*fw_; }
    int r2y( int row ) const            { return row*fh_; }
    pixel_t color( int col ) const      { return palette_[col]; }
    pixel_t& pixel( int x, int y )      { return fb_[y*w_ + x]; }
    cell_t& cell( int r, int c )        { return cells_[r*rows_ + c]; }

    void draw( );
    void op_putc( int row, int col, cell_t charattr );
    void op_puts( int row, int col, cell_t* strattr, int len );
    void op_scroll_up( int top, int bottom, int n );
    void op_scroll_down( int top, int bottom, int n );
    void op_cursor_erase( );
    void op_cursor_move( int row, int col, int height );
    void op_cursor_draw( int row, int col, int height );
    void op_clear( int t,int l, int b,int r, cell_t chattr );
    void op_bmove( int row,int col, int t,int l, int b,int r );
    void op_set_font( int fw, int fh, int count, uchar* data );

    void mark_damage( );
    static void refresh_callback( void* v );

private:
    uchar       *   fdata_;             // Font data array
    int             fw_, fh_;           // Font width/heigth in bits
    int             w_, h_;             // Frame buffer width/height
    int             rows_, cols_;       // Rows/Columns
    cell_t          blank_;             // Cell used in deletetions
    cell_t      *   cells_;             // Character screen cells
    pixel_t     *   fb_;                // Framebuffer
    ulong           palette_[16];       // Color palette
    int             cur_r_, cur_c_;     // Current cursor position
    int             cur_h_;             // Cursor height (0 if hidden)
    int             damages_;           // Count of damages since last redraw
    clock_t         last_damage_;       // Time of the last damage
    clock_t         last_redraw_;       // Time of last redraw
    bool            mark_mode_;         // True if in selection mode
};

#endif // COLINUX_USER_CONSOLE_FB_VIEW_H
