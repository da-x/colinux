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
#include "fb_view.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/**
 * EGA/VGA default color pallete.
 *
 * The format is 0xRRGGBB (RR=Blue, GG=Green, BB=Blue). It's the format
 * of the FLTK raw data array of 32 bits color depth in little-endian
 * machines.
 *
 * NOTE: Didn't found nothing in FLTK that says this format is inverted
 *       on big-endian machines, so this needs to be checked when coLinux
 *       gets ported to those (in some far future, for now).
 */
static const ulong default_vga_color_palette[16] = {
    /* Dim Colors */
    0x000000,0xAA0000,0x00AA00,0xAAAA00,    // Black, Red    , Green, Brown
    0x0000AA,0xAA00AA,0x00AAAA,0xAAAAAA,    // Blue , Magenta, Cyan , White
    /* Bright Colors */
    0x555555,0xFF5555,0x55FF55,0xFFFF55,    // Black, Red    , Green, Brown
    0x5555FF,0xFF55FF,0x55FFFF,0xFFFFFF     // Blue , Magenta, Cyan , White
};

/**
 * Default VGA font, in default_font.cpp.
 */
extern int default_vga_font_width;
extern int default_vga_font_height;
extern unsigned char default_vga_font_data[];

/**
 * Class constructor.
 *
 * The widget size will be used as the dimensions for the framebuffer.
 * On resize, the framebuffer will resize also (which means this is to be
 * used inside a Fl_Scroll widget or we would suffer of very bad performance
 * when resizing the FLTK console).
 */
console_fb_view::console_fb_view( int x, int y, int w, int h )
    : super_( x,y, w,h )
    , fdata_( default_vga_font_data )
    , fw_( default_vga_font_width )
    , fh_( default_vga_font_height )
    , w_( w )
    , h_( h )
    , rows_( h/default_vga_font_height )
    , cols_( w/default_vga_font_width )
    , blank_( 0x0720 )
    , cur_r_( 0 )
    , cur_c_( 0 )
    , cur_h_( 0 )
    , damages_( 0 )
    , last_damage_( 0 )
    , last_redraw_( 0 )
    , mark_mode_( false )
{
    memcpy( palette_, default_vga_color_palette, sizeof(palette_) );
    fb_    = new pixel_t[ w_*h_ ];
    cells_ = new cell_t[ rows_*cols_ ];
    // Add idle callback for handling refresh
    Fl::add_check( refresh_callback, this );
}

/**
 * Returns virtual mouse position, given its widget position.
 */
#if 0
int console_fb_view::mouse_x( int mx ) const
{
    return mx - x();
}
int mouse_y( int my ) const
{
    return my - y();
}
#endif
/**
 * Class destructor.
 *
 * Release resources used.
 */
console_fb_view::~console_fb_view( )
{
    Fl::remove_check( refresh_callback, this );
    delete[] cells_;
    delete[] fb_;
    if ( fdata_ != default_vga_font_data )
        delete[] fdata_;
}

/**
 * Increments 'damages_' counter and sets 'last_damage_' time.
 */
void console_fb_view::mark_damage( )
{
    ++damages_;
    last_damage_ = clock();
}

/**
 * Checks if screen needs refresh, ordering the redraw if needed.
 *
 * To avoid wasting time redrawing too often, we only refresh the screen
 * after a damage if either of these happens:
 *      1) The number of 'damages_' is bigger than 2000 (2000 chars)
 *      2) More than 50 msecs elapsed since the last damage
 *      3) More than 100 msecs elapsed since the the last redraw
 *
 * FIXME: Have some way to test this harcoded values and/or check if
 *          there are better combinations.
 */
void console_fb_view::refresh_callback( void* user_data )
{
    self_t& this_ = *(self_t*)user_data;

    // Screen needs re-painting?
    if ( this_.damages_ )
    {
        clock_t now = clock();
        clock_t msecs_since_damage = 1000*(now - this_.last_damage_)/CLOCKS_PER_SEC;
        clock_t msecs_since_redraw = 1000*(now - this_.last_redraw_)/CLOCKS_PER_SEC;

        // Only redraw if needed
        if ( this_.damages_ > 2000 )
        {
            this_.redraw( );
            this_.damages_ = 0;
        }
        else if ( msecs_since_damage > 50 )
        {
            this_.redraw( );
            this_.damages_ = 0;
        }
        else if ( msecs_since_redraw > 100 )
        {
            this_.redraw( );
            this_.damages_ = 0;
        }
    }
}

/**
 * Setup initial console screen.
 */
void console_fb_view::attach( co_console_t *console )
{
    // Reset internal data structures
    fdata_ = default_vga_font_data;
    fw_    = default_vga_font_width;
    fh_    = default_vga_font_height;
    cols_  = console->x;
    rows_  = console->y;
    w_     = cols_*fw_;
    h_     = rows_*fh_;
    cur_r_ = console->cursor.y;
    cur_c_ = console->cursor.x;
    cur_h_ = console->cursor.height;
    memcpy( palette_, default_vga_color_palette, sizeof(palette_) );

    delete[] fb_;
    delete[] cells_;
    fb_    = new pixel_t[ w_ * h_ ];
    cells_ = new cell_t[ rows_ * cols_ ];

    // Copy console screen
    for( int j=0; j < rows_; ++j )
        op_puts( j,0, (cell_t *) &console->screen[j*cols_], cols_ );

    // Resize window
    size( w_, h_ );
}

/**
 * Signal terminal was dettached.
 */
void console_fb_view::dettach( )
{
    cur_h_ = 0; // Hide cursor
    mark_damage( );
}

/**
 * Draws the console screen.
 */
void console_fb_view::draw( )
{
    // Set time of this redraw
    last_redraw_ = clock();

    // Get damaged area
    int dx,dy, dw,dh;
    fl_clip_box( x(),y(), w(),h(), dx,dy, dw,dh );
    if ( !dw || !dh )
        return; // Nothing needs to be drawn

    // Calculate what needs to be drawn
    int dx2 = dx + dw;
    int dy2 = dy + dh;
    if ( dx < x() )         dx = x();
    if ( dy < y() )         dy = y();
    if ( dx2 > x() + w_ )   dx2 = x() + w_;
    if ( dy2 > y() + h_ )   dy2 = y() + h_;
    dw = dx2 - dx;
    dh = dy2 - dy;

    // Clip area & draw
    fl_push_clip( dx,dy, dw,dh );

//    fl_draw_image( copy_scanline,this, x(),y(), w_,h_, 4 );
    fl_draw_image( (uchar*)fb_, x(),y(), w_,h_, 4 );

    // Draw cursor (if not hidden)
    if ( cur_h_ )
    {
        fl_color( FL_WHITE );
        fl_rectf( x()+c2x(cur_c_), y()+r2y(cur_r_+1)-cur_h_-1, fw_, cur_h_ );
    }

    fl_pop_clip( );
}

/**
 * Print character/attribute at position (row,col).
 */
void console_fb_view::op_putc( int row, int col, cell_t chattr )
{
    // Decode char & attributes
    const char ch = chattr & 0xFF;
    const int  fg = (chattr >>  8) & 0x0F;
    const int  bg = (chattr >> 12) & 0x0F;
    // Font metrics & get pointer to character font data
    const int fbw = (fw_-1)/8 + 1;          // bytes/char scan line
    const uchar* chdata = fdata_ + (ch&0xFF)*fh_*fbw; // font data

    /*
     * FIXME:
     *      I'm sure there are a ton of optimized ways of doing this.
     *      Patches are welcome ;-)
     */
    for ( int h=0; h < fh_; ++h )
        for( int b=0; b < fw_; ++b )
        {
            const uchar cs = chdata[ h*fbw + b/8 ];
            const bool bit = cs & (1 << (7-(b%8)));
            pixel( c2x(col)+b, r2y(row)+h ) = bit? color(fg) : color(bg);
        }
    cell( row, col ) = chattr;

    // Signal screen was damaged
    mark_damage( );
}

/**
 * Prints string 'str' at position (row,col).
 */
void console_fb_view::op_puts( int row, int col, cell_t* str, int len )
{
    for ( int i=0; i < len; ++i )
        op_putc( row,col+i, str[i] );
}

/**
 * Fills rectangle (t,l)->(b,r) with character/attribute 'chattr'.
 */
void console_fb_view::op_clear( int t,int l, int b,int r, cell_t chattr )
{
    /* FIXME: Optimize this. */
    for ( int j = t; j <= b; ++j )
        for ( int i = l; i <= r; ++i )
            op_putc( j,i, chattr );
}

/**
 * Some utility templates for type based memcpy/memset.
 * This is just to avoid including C++ headers.
 */
template<typename T>
void copy_mem( T* dest, const T* src, unsigned count )
{
    for ( int i = 0; i < count; ++i )
        dest[i] = src[i];
}

template<typename T>
void set_mem( T* mem, const T& value, unsigned count )
{
    for ( int i = 0; i < count; ++i )
        mem[i] = value;
}

/**
 * Scrolls rows from [top..bottom], 'n' lines up.
 *
 * FIXME: Implement backlog.
 */
void console_fb_view::op_scroll_up( int top, int bottom, int n )
{
    assert( n > 0 && 0 <= top && top <= bottom && bottom < rows_ );

    for ( int r = top; r <= bottom; ++r )
    {
        // Move row
        if ( r - n >= 0 )
        {
            copy_mem<cell_t>( &cell(r-n,0), &cell(r,0), cols_ );
            copy_mem<pixel_t>( &pixel(0,r2y(r-n)), &pixel(0,r2y(r)), w_*fh_ );
        }
        // Erase row (if at end)
        if ( r + n >= bottom )
            op_clear( r,0, r,cols_, blank_ );
    }
}

/**
 * Scrolls rows from [top..bottom], 'n' lines down.
 */
void console_fb_view::op_scroll_down( int top, int bottom, int n )
{
    assert( n > 0 && 0 <= top && top <= bottom && bottom < rows_ );

    for ( int r = bottom; r >= top; --r )
    {
        // Move row
        if ( r + n < rows_ )
        {
            copy_mem<cell_t>( &cell(r+n,0), &cell(r,0), cols_ );
            copy_mem<pixel_t>( &pixel(0,r2y(r+n)), &pixel(0,r2y(r)), w_*fh_ );
        }
        // Erase row (if at beginning)
        if ( r - n + 1 >= 0 )
            op_clear( r,0, r,cols_, blank_ );
    }
}

/**
 * Move contents of rectangle (t,l)-->(b,r) to position (row,col).
 * The rect is moved to a local bufer and then "pasted" over the destination.
 */
void console_fb_view::op_bmove( int row,int col, int t,int l, int b,int r )
{
    printf( "op_bmove( %d,%d, %d,%d-%d,%d )\n", row,col, t,l, b,r );

    const int cols = r - l + 1;
    const int rows = b - t + 1;
    cell_t* buf = new cell_t[ cols*rows ];

    // "Copy" area
    for ( int j = t; j <= b; ++j )
        copy_mem<cell_t>( &buf[(j-t)*cols], &cell(j,l), cols );

    // "Paste" into the new position
    for ( int j = row; j < row + rows; ++j )
        op_puts( j,col, &buf[(j-row)*cols], cols );

    // Release temporary buffer
    delete[] buf;
}

/**
 * Hide cursor.
 */
void console_fb_view::op_cursor_erase( )
{
    if ( cur_h_ == 0 )
        return; // already hidden
    cur_h_ = 0;

    // Signal screen was damaged
    mark_damage( );
}

/**
 * Show cursor at (row,col) with height 'height'.
 */
void console_fb_view::op_cursor_draw( int row, int col, int cur_h )
{
    cur_r_ = row;
    cur_c_ = col;
    cur_h_ = cur_h;

    // Signal screen was damaged
    mark_damage( );
}

/**
 * Move cursor to (row,col) with height 'height'.
 */
void console_fb_view::op_cursor_move( int row, int col, int height )
{
    cur_r_ = row;
    cur_c_ = col;

    // Signal screen was damaged
    mark_damage( );
}

/**
 * Set user font selected in the colinux instance.
 */
void console_fb_view::op_set_font( int fw, int fh, int count, uchar* data )
{
    /*
     * For now, resize screen, but not number of of rows/columns
     */

    printf( "op_set_font( %d,%d, %d )\n", fw,fh, count );

    // Reset internal data structures
    if ( fdata_ != default_vga_font_data )
        delete[] fdata_;
    const int fwb = (fw-1)/8 + 1;
    const int data_size = count*fwb*fh;
    fdata_ = new uchar[data_size];
    memcpy( fdata_, data, data_size );
    fw_    = fw;
    fh_    = fh;
    w_     = cols_*fw_;
    h_     = rows_*fh_;

    delete[] fb_;
    fb_    = new pixel_t[ w_ * h_ ];

    // Redraw the screen
    cell_t* line = new cell_t[cols_];
    for( int j=0; j < rows_; ++j )
    {
        copy_mem<cell_t>( line, &cell(j,0), cols_ );
        op_puts( j,0, line, cols_ );
    }
    delete[] line;

    // Resize window, if needed
    if ( w() != w_ || h() != h_ )
        this->size( w_, h_ );
}

/**
 * Handle console output events.
 */
void console_fb_view::handle_console_event( co_console_message_t *msg )
{
    switch ( msg->type )
    {
    case CO_OPERATION_CONSOLE_SCROLL_UP:
        op_scroll_up( msg->scroll.top, msg->scroll.bottom, msg->scroll.lines );
        break;
    case CO_OPERATION_CONSOLE_SCROLL_DOWN:
        op_scroll_down( msg->scroll.top, msg->scroll.bottom, msg->scroll.lines );
        break;
    case CO_OPERATION_CONSOLE_PUTCS:
        op_puts( msg->putcs.y, msg->putcs.x,
                    (cell_t *)msg->putcs.data, msg->putcs.count );
        break;
    case CO_OPERATION_CONSOLE_PUTC:
        op_putc( msg->putc.y, msg->putc.x, msg->putc.charattr );
        break;
	case CO_OPERATION_CONSOLE_CURSOR_ERASE:
        op_cursor_erase( );
        break;
	case CO_OPERATION_CONSOLE_CURSOR_MOVE:
        op_cursor_move( msg->cursor.y, msg->cursor.x, msg->cursor.height );
		break;
    case CO_OPERATION_CONSOLE_CURSOR_DRAW:
        op_cursor_draw( msg->cursor.y, msg->cursor.x, msg->cursor.height );
        break;
	case CO_OPERATION_CONSOLE_CLEAR:
        op_clear( msg->clear.top, msg->clear.left,
                    msg->clear.bottom, msg->clear.right,
                    msg->clear.charattr );
		break;
	case CO_OPERATION_CONSOLE_BMOVE:
        op_bmove( msg->bmove.row, msg->bmove.column,
                    msg->bmove.top, msg->bmove.left,
                    msg->bmove.bottom, msg->bmove.right );
        break;
	case CO_OPERATION_CONSOLE_FONT_OP:
        printf( "CO_OPERATION_CONSOLE_FONT_OP\n" );
        op_set_font( msg->font.width, msg->font.height,
                     msg->font.count, msg->font.data );
        break;

	case CO_OPERATION_CONSOLE_SCROLLDELTA:
        printf( "CO_OPERATION_CONSOLE_SCROLLDELTA - %d lines\n", msg->value );
        break;
	case CO_OPERATION_CONSOLE_SET_ORIGIN:
        printf( "CO_OPERATION_CONSOLE_SET_ORIGIN\n" );
        break;

	case CO_OPERATION_CONSOLE_STARTUP:
        printf( "CO_OPERATION_CONSOLE_STARTUP\n" );
        break;
	case CO_OPERATION_CONSOLE_INIT:
        printf( "CO_OPERATION_CONSOLE_INIT\n" );
        break;
	case CO_OPERATION_CONSOLE_DEINIT:
        printf( "CO_OPERATION_CONSOLE_DEINIT\n" );
        break;
	case CO_OPERATION_CONSOLE_SWITCH:
        printf( "CO_OPERATION_CONSOLE_SWITCH\n" );
        break;
	case CO_OPERATION_CONSOLE_BLANK:
        printf( "CO_OPERATION_CONSOLE_BLANK\n" );
        break;
	case CO_OPERATION_CONSOLE_SET_PALETTE:
        printf( "CO_OPERATION_CONSOLE_SET_PALETTE\n" );
        break;
	case CO_OPERATION_CONSOLE_SAVE_SCREEN:
        printf( "CO_OPERATION_CONSOLE_SAVE_SCREEN\n" );
        break;
	case CO_OPERATION_CONSOLE_INVERT_REGION:
        printf( "CO_OPERATION_CONSOLE_INVERT_REGION\n" );
		break;
	}
}

/**
 * Mark text from (x1,y1) to (x2,y2).
 */
void console_fb_view::set_marked_text( int x1,int y1, int x2,int y2 )
{
}

/**
 * Get current marked text.
 */
const char* console_fb_view::get_marked_text( ) const
{
    return "";
}

/**
 * Clear current selection.
 */
void console_fb_view::clear_marked_text( )
{
}
