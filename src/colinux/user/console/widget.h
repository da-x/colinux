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
#ifndef __COLINUX_USER_CONSOLE_WIDGET_H__
#define __COLINUX_USER_CONSOLE_WIDGET_H__

extern "C" {
    #include <colinux/common/console.h>
}
#include <FL/Fl_Widget.H>


/**
 * Client window responsible for drawing the colinux console.
 */
class console_widget : public Fl_Widget
{
    typedef Fl_Widget           super_;
    typedef console_widget      self_t;
public:
     console_widget( int x, int y, int w, int h );
    ~console_widget( );

    void attach( co_console_t *_console );
    void dettach( );

    co_rc_t handle_console_event( co_console_message_t *message );

    void set_font( Fl_Font face, int size );
    void set_font_face( Fl_Font face )      { set_font(face,font_size_); }
    void set_font_size( int size )          { set_font(font_face_,size); }
    Fl_Font get_font_face( ) const          { return font_face_; }
    int get_font_size( ) const              { return font_size_; }

    int term_x( ) const                     { return x()+4; }
    int term_y( ) const                     { return y()+4; }
    int term_w( ) const                     { return cols_*cw_; }
    int term_h( ) const                     { return lines_*ch_; }
    int x2c( int px ) const                 { return (px-term_x())/cw_; }
    int y2l( int py ) const                 { return (py-term_y())/ch_; }
    int c2x( int col ) const                { return term_x()+cw_*col; }
    int l2y( int line ) const               { return term_y()+ch_*line; }

    void set_marked_text( int x1, int y1, int x2, int y2 );
    const char* get_marked_text( );
    void clear_marked( );

private:
    void draw( );
    void damage_console( int col, int line, int cw, int lh );
    void draw_normal_row( int line, int min, int max );
    void draw_marked_row_lr( int line, int min, int max, int lmark, int rmark );
    void draw_marked_row_l( int line, int min, int max, int lmark );
    void draw_marked_row_r( int line, int min, int max, int rmark );
    void draw_inverted_row( int line, int min, int max );

    static void blink_handler( void* data );

    Fl_Font             font_face_;             // Font face used
    int                 font_size_;             // Font size used
    int                 cw_, ch_;               // Char width/height (pixels)
    int                 cols_, lines_;          // Console dimensions
    int                 ox_, oy_;               // Console top/left corner
    int                 cur_c_, cur_l_;         // Cursor position (col,line)
    int                 cur_h_;                 // Cursor height (in pixels)
    co_console_t    *   console_;               // Helper console object
    double              cursor_blink_interval_; // Blinks per second
    bool                cursor_blink_state_;    // Current blink state
    bool                mark_mode_;             // true if in selection mode
    int                 ml_,mt_,mr_,mb_;        // Current selection
    char *              sel_text_;              // Selected text
};


#endif
