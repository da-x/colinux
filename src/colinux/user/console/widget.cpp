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
#include "widget.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <assert.h>
#include <stdlib.h>


/**
 * Default VGA colors.
 *
 * It follows the Fl_Color convention, that is: 0xRRGGBB00.
 * Black is a special case, because 0 is FL_FOREGROUND_COLOR.
 */
static int default_red[] = {0x00,0xaa,0x00,0xaa,0x00,0xaa,0x00,0xaa,
                            0x55,0xff,0x55,0xff,0x55,0xff,0x55,0xff};

static int default_grn[] = {0x00,0x00,0xaa,0xaa,0x00,0x00,0xaa,0xaa,
                            0x55,0x55,0xff,0xff,0x55,0x55,0xff,0xff};

static int default_blu[] = {0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,
                            0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff};

/**
 * Helper for seting the console VGA color.
 *
 * There are 16 VGA colors, from 0-black to 15-white.
 */
#define VGA_COLOR(i) fl_color(default_blu[i], default_grn[i], default_red[i])

/**
 * Class constructor.
 *
 * Initializes internal engine.
 */
console_widget::console_widget( int x, int y, int w, int h )
    : super_( x,y, w,h )
    , font_face_( FL_SCREEN )
    , font_size_( 18 )
    , cols_( 80 )
    , lines_( 25 )
    , cur_c_( 0 )
    , cur_l_( 0 )
    , cur_h_( 2 )
    , console_( 0 )
    , cursor_blink_interval_( 0.2 )     // Blink every 4/10 of second
    , cursor_blink_state_( 1 )
    , mark_mode_( false )
    , sel_text_( 0 )
{
    // Get character dimensions
    fl_font( font_face_, font_size_ );
    cw_ = static_cast<int>( fl_width('M') );
    ch_ = fl_height( );

    // Set some bits
    type( FL_NO_LABEL );
}

/**
 * Class destructor.
 *
 * Release any resources used.
 */
console_widget::~console_widget( )
{
    dettach( );
    if ( sel_text_ )
        delete[] sel_text_;
}

/**
 * Set font used for drawing the console window.
 */
void console_widget::set_font( Fl_Font face, int size )
{
    fl_font( face, size );
    font_face_ = face;
    font_size_ = size;
    // Get character & cursor dimensions
    cw_ = (int)fl_width('W');
    ch_ = fl_height( );
}

/**
 * Use given console object to draw console & start blink handler.
 */
void console_widget::attach( co_console_t * console )
{
    console_ = console;
    cols_ = console->x;
    lines_ = console->y;

    damage_console( 0,0, cols_,lines_ );

    /*
     * On attach, there is no cursor position defined, so use the last and
     * wait for it to be set after the next console event. It makes re-attach
     * look better ;)
     * This needs to be addressed in the kernel code.
     */

    // Set a timeout to make the cursor blink
    Fl::add_timeout( cursor_blink_interval_, blink_handler, this );
}

/**
 * Just stop blink handler, as we want to let the user see the last console
 * if something has gone wrong.
 */
void console_widget::dettach( )
{
    // Just remove timeout to make cursor blink
    Fl::remove_timeout( blink_handler, this );
    cursor_blink_state_ = 1;
}

/**
 * Timeout handler to make the cursor blink.
 *
 * For the cursor blink we would do: cursor_blink_state = !cursor_blink_state
 * However, we need to fix a problem with console_idle() which causes
 * timers not to execute unless there are input events.
 *
 * nlucas: Right now it works, but only after the user presses the first key.
 */
void console_widget::blink_handler( void* data )
{
    self_t * this_ = reinterpret_cast<self_t *>( data );
    this_->cursor_blink_state_ = !this_->cursor_blink_state_;
    if ( this_->console_ )
        this_->damage_console( this_->cur_c_,this_->cur_l_, 1,1 );
    Fl::repeat_timeout( this_->cursor_blink_interval_, blink_handler, data );
}

/**
 * Force a refresh of the rectangle (in columns,lines coordinates).
 */
void console_widget::damage_console( int col,int line, int nCols,int nLines )
{
    damage( 1, c2x(col), l2y(line), nCols*cw_, nLines*ch_ );
}

/**
 * Draw console.
 */
void console_widget::draw( )
{
    // Check we have a console structure
    if ( console_ == NULL )
    {
        int cx,cy, cw,ch;
        // Fill damaged box with the VGA black color (color 0)
        fl_clip_box( x(),y(), w(),h(), cx,cy, cw,ch );
        VGA_COLOR( 0 );
        fl_rectf( cx,cy, cw,ch );
        return;
    }

    // Get terminal dimensions & origin in pixels
    const int ox = term_x();    const int oy = term_y();
    const int tw = term_w();    const int th = term_h();

    // Get damaged rect (dx,dy)-(dw,dh)
    int dx,dy, dw,dh;
    fl_clip_box( x(),y(), w(),h(), dx,dy, dw,dh );

    // Limit drawing to damaged area
    fl_push_clip( dx,dy, dw,dh );

    // Draw outside borders & limit to term area
    VGA_COLOR( 0 ); // Select VGA black color
    if ( dy < oy )
    {
        fl_rectf( x(), dy, w(), oy-dy );
        dy = oy;
    }
    if ( dy+dh > oy+th )
    {
        fl_rectf( x(), oy+th, w(), y()+h()-oy-th );
        dh = oy + th - dy;
    }
    if ( dx < ox )
    {
        fl_rectf( dx, oy, ox-dx, th );
        dx = ox;
    }
    if ( dx+dw > ox+tw )
    {
        fl_rectf( ox+tw, oy,    x()+w()-ox-tw, th );
        dw = ox + tw - dx;
    }

    // Convert it to columns,lines coordinates
    int col1,lin1, col2,lin2;
    col1 = x2c( dx );           lin1 = y2l( dy );
    col2 = x2c( dx+dw );        lin2 = y2l( dy+dh );

    // Make sure we use the selected font
    fl_font( font_face_, font_size_ );

    // Start drawing, from top to bottom
    for ( int j = lin1; j < lin2; ++j )
    {
        if ( mark_mode_ )
        {
            // Draw acording to current selection
            if ( j == mt_ && j == mb_ )
                draw_marked_row_lr( j, col1, col2, ml_, mr_ );
            else if ( j == mt_ )
                draw_marked_row_l( j, col1, col2, ml_ );
            else if ( j == mb_ )
                draw_marked_row_r( j, col1, col2, mr_ );
            else if ( mt_ < j && j < mb_ )
                draw_inverted_row( j, col1, col2 );
            else
                draw_normal_row( j, col1, col2 );
        }
        else
        {
            // Draw normal mode row
            draw_normal_row( j, col1, col2 );
        }

        // Draw cursor, if visible in this row
        if ( cursor_blink_state_ && j == cur_l_
             && cur_c_ >= col1 && cur_c_ < col2 )
        {
            // Use VGA color 15 (White)
            VGA_COLOR( 15 );
            fl_rectf( c2x(cur_c_), l2y(cur_l_ + 1) - cur_h_ - 1, cw_, cur_h_ );
        }
    }

    // All done, remove cliping
    fl_pop_clip( );
}

/**
 * Draw row in normal mode.
 */
void console_widget::draw_normal_row( int line, int min, int max )
{
    const co_console_cell_t* row = &console_->screen[line*cols_];
    char buf[256];
    int i = min;
    while ( i < max )
    {
        int end = i;
        while ( end < max && row[i].attr == row[end].attr )
        {
            buf[end - i] = row[end].ch;
            ++end;
        }

        VGA_COLOR( (row[i].attr >> 4) & 0x0F );
        fl_rectf( c2x(i), l2y(line), cw_*(end-i), ch_ );
        VGA_COLOR( row[i].attr & 0x0F );
        fl_draw( buf, end-i, c2x(i), l2y(line+1) - fl_descent() );

        i = end;
    }
}

/**
 * Draw row with the attributes inverted.
 */
void console_widget::draw_inverted_row( int line, int min, int max )
{
    const co_console_cell_t* row = &console_->screen[line*cols_];
    char buf[256];
    int i = min;
    while ( i < max )
    {
        int end = i;
        while ( end < max && row[i].attr == row[end].attr )
        {
            buf[end - i] = row[end].ch;
            ++end;
        }

        VGA_COLOR( row[i].attr & 0x0F );
        fl_rectf( c2x(i), l2y(line), cw_*(end-i), ch_ );
        VGA_COLOR( (row[i].attr >> 4) & 0x0F );
        fl_draw( buf, end-i, c2x(i), l2y(line+1) - fl_descent() );

        i = end;
    }
}

/**
 * Draw marked row at left.
 */
void console_widget::draw_marked_row_l( int line, int min, int max, int lmark )
{
    const co_console_cell_t* row = &console_->screen[line*cols_];
    char buf[256];
    bool invert = false;
    int i = min;
    while ( i < max )
    {
        int end = i;
        while ( end < max && end < lmark && row[i].attr == row[end].attr )
        {
            buf[end - i] = row[end].ch;
            ++end;
        }

        if ( invert )
        {
            VGA_COLOR( row[i].attr & 0x0F );
            fl_rectf( c2x(i), l2y(line), cw_*(end-i), ch_ );
            VGA_COLOR( (row[i].attr >> 4) & 0x0F );
            fl_draw( buf, end-i, c2x(i), l2y(line+1) - fl_descent() );
        }
        else
        {
            VGA_COLOR( (row[i].attr >> 4) & 0x0F );
            fl_rectf( c2x(i), l2y(line), cw_*(end-i), ch_ );
            VGA_COLOR( row[i].attr & 0x0F );
            fl_draw( buf, end-i, c2x(i), l2y(line+1) - fl_descent() );
        }

        if ( end == lmark )
        {
            lmark = max;
            invert = true;
        }

        i = end;
    }
}

/**
 * Draw marked row at right.
 */
void console_widget::draw_marked_row_r( int line, int min, int max, int rmark )
{
    const co_console_cell_t* row = &console_->screen[line*cols_];
    char buf[256];
    bool invert = true;
    int i = min;
    while ( i < max )
    {
        int end = i;
        while ( end < max && end < rmark && row[i].attr == row[end].attr )
        {
            buf[end - i] = row[end].ch;
            ++end;
        }

        if ( invert )
        {
            VGA_COLOR( row[i].attr & 0x0F );
            fl_rectf( c2x(i), l2y(line), cw_*(end-i), ch_ );
            VGA_COLOR( (row[i].attr >> 4) & 0x0F );
            fl_draw( buf, end-i, c2x(i), l2y(line+1) - fl_descent() );
        }
        else
        {
            VGA_COLOR( (row[i].attr >> 4) & 0x0F );
            fl_rectf( c2x(i), l2y(line), cw_*(end-i), ch_ );
            VGA_COLOR( row[i].attr & 0x0F );
            fl_draw( buf, end-i, c2x(i), l2y(line+1) - fl_descent() );
        }

        if ( end == rmark )
        {
            rmark = max;
            invert = false;
        }

        i = end;
    }
}

/**
 * Draw marked row at left and right.
 */
void console_widget::draw_marked_row_lr( int line, int min, int max,
                                            int lmark, int rmark )
{
    const co_console_cell_t* row = &console_->screen[line*cols_];
    char buf[256];
    bool invert = false;
    int i = min;
    while ( i < max )
    {
        int end = i;
        while ( end < max && end < lmark && end < rmark
                && row[i].attr == row[end].attr )
        {
            buf[end - i] = row[end].ch;
            ++end;
        }

        if ( invert )
        {
            VGA_COLOR( row[i].attr & 0x0F );
            fl_rectf( c2x(i), l2y(line), cw_*(end-i), ch_ );
            VGA_COLOR( (row[i].attr >> 4) & 0x0F );
            fl_draw( buf, end-i, c2x(i), l2y(line+1) - fl_descent() );
        }
        else
        {
            VGA_COLOR( (row[i].attr >> 4) & 0x0F );
            fl_rectf( c2x(i), l2y(line), cw_*(end-i), ch_ );
            VGA_COLOR( row[i].attr & 0x0F );
            fl_draw( buf, end-i, c2x(i), l2y(line+1) - fl_descent() );
        }

        if ( end == lmark )
        {
            lmark = max;
            invert = true;
        }
        if ( end == rmark )
        {
            rmark = max;
            invert = false;
        }

        i = end;
    }
}

/**
 * Filter the console events to mark as "damaged" the changed parts.
 */
co_rc_t console_widget::handle_console_event( co_console_message_t *msg )
{
    assert( console_ );
    co_rc_t rc;

    rc = co_console_op( console_, msg );
    if ( !CO_OK(rc) )
        return rc;

    switch ( msg->type )
    {
    case CO_OPERATION_CONSOLE_SCROLL_UP:
    case CO_OPERATION_CONSOLE_SCROLL_DOWN:
        {
            unsigned long t = msg->scroll.top;      // Start of scroll region (row)
            unsigned long b = msg->scroll.bottom+1; // End of scroll region (row)
            damage_console( 0, t, cols_, b - t + 1 );
            break;
        }
    case CO_OPERATION_CONSOLE_PUTCS:
        {
            int x = msg->putcs.x;
            int y = msg->putcs.y;
            int count = msg->putcs.count;
            if ( x + count > cols_ )
                damage_console( 0,y, cols_,(x+count)/cols_ + 1 );
            else
                damage_console( x,y, count,1 );
            break;
        }
    case CO_OPERATION_CONSOLE_PUTC:
        {
            int x = msg->putc.x;
            int y = msg->putc.y;
            damage_console( x,y, 1,1 );
            break;
        }
    case CO_OPERATION_CONSOLE_CURSOR_MOVE:
    case CO_OPERATION_CONSOLE_CURSOR_DRAW:
    case CO_OPERATION_CONSOLE_CURSOR_ERASE:
        {
            damage_console( cur_c_,cur_l_, 1,1 );
            cur_c_ = console_->cursor.x;
            cur_l_ = console_->cursor.y;
            // Don't damage new pos because it will be on next blink
            break;
        }
    case CO_OPERATION_CONSOLE_CLEAR:
        {
            unsigned t = msg->clear.top;
            unsigned l = msg->clear.left;
            unsigned b = msg->clear.bottom;
            unsigned r = msg->clear.right;
            damage_console( l,t, r-l+1,b-t+1 );
        }
    case CO_OPERATION_CONSOLE_BMOVE:
        {
            unsigned y = msg->bmove.row;
            unsigned x = msg->bmove.column;
            unsigned t = msg->bmove.top;
            unsigned l = msg->bmove.left;
            unsigned b = msg->bmove.bottom;
            unsigned r = msg->bmove.right;
            damage_console( x,y, r-l+1,b-t+1 );
            damage_console( l,t, r-l+1,b-t+1 );
        }
    case CO_OPERATION_CONSOLE_STARTUP:
    case CO_OPERATION_CONSOLE_INIT:
    case CO_OPERATION_CONSOLE_DEINIT:
    case CO_OPERATION_CONSOLE_SWITCH:
    case CO_OPERATION_CONSOLE_BLANK:
    case CO_OPERATION_CONSOLE_FONT_OP:
    case CO_OPERATION_CONSOLE_SET_PALETTE:
    case CO_OPERATION_CONSOLE_SCROLLDELTA:
    case CO_OPERATION_CONSOLE_SET_ORIGIN:
    case CO_OPERATION_CONSOLE_SAVE_SCREEN:
    case CO_OPERATION_CONSOLE_INVERT_REGION:
        break;
    }

    return CO_RC(OK);
}

/**
 * Select marked text.
 */
void console_widget::set_marked_text( int x1,int y1, int x2,int y2 )
{
    const int oy = term_y();    const int tw = term_w();
    const int ox = term_x();    const int th = term_h();

    // Damage last selection
    damage_console( 0,mt_, cols_,mb_ );

    // Assure they are on the scrern terminal
    if ( y1 < oy )          y1 = oy;
    else if ( y1 > oy+th )  y1 = oy + th;
    if ( y2 < oy )          y2 = oy;
    else if ( y2 > oy+th )  y2 = oy + th;
    if ( x1 < ox )          x1 = ox;
    else if ( x1 > ox+tw )  x1 = ox + tw;
    if ( x2 < ox )          x2 = ox;
    else if ( x2 > ox+tw )  x2 = ox + tw;

    // Convert to columns,lines coordinates
    ml_ = x2c( x1 );        mt_ = y2l( y1 );
    mr_ = x2c( x2 );        mb_ = y2l( y2 );

    // Ensure point1 is less than point2
    if ( ml_ + mt_*cols_ > mr_ + mb_*cols_ )
    {
        int tmp;
        // Swap points
        tmp = mr_; mr_ = ml_; ml_ = tmp;
        tmp = mb_; mb_ = mt_; mt_ = tmp;
    }

    // Damage terminal & mark selection mode
    damage_console( 0,mt_, cols_,mb_ );
    mark_mode_ = true;
}

/**
 * Clear current marked selection.
 *
 * Just sets the two offsets to the same value.
 */
void console_widget::clear_marked( )
{
    mark_mode_ = false;
    if ( sel_text_ )
    {
        delete[] sel_text_;
        sel_text_ = 0;
    }
}

/**
 * Return last marked text.
 */
const char* console_widget::get_marked_text( )
{
    if ( !console_ )
        return 0;   // Nothing to copy

    // Delete previous selection, if any
    if ( sel_text_ )
        delete[] sel_text_;
    sel_text_ = 0;

    // Alloc buffer and copy text
    int begin = ml_ + mt_*cols_;
    int end   = mr_ + mb_*cols_;

    if ( end == begin )
        return 0;   // Nothing to copy

    // 1 extra char per line and 1 for the terminating '\0'
    sel_text_= new char[ end - begin + (mb_ - mt_ + 1) + 1 ];
    char* s = sel_text_;
    while ( begin < end )
    {
        *s = console_->screen[begin].ch;
        if ( *s < 32 )
            *s = ' ';
        ++s;
        ++begin;
        if ( (begin % cols_) == 0 )
            *s++ = '\n';
    }

    return sel_text_;
}
