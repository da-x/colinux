/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#include "screen_cocon.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <stdlib.h>
//#include <string.h>



/* ----------------------------------------------------------------------- */
/*
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
static __u32 default_color_palette[16] = {
    /* Dim Colors */
    0x000000,0xAA0000,0x00AA00,0xAAAA00,    // Black, Red    , Green, Brown
    0x0000AA,0xAA00AA,0x00AAAA,0xAAAAAA,    // Blue , Magenta, Cyan , White
    /* Bright Colors */
    0x555555,0xFF5555,0x55FF55,0xFFFF55,    // Black, Red    , Green, Brown
    0x5555FF,0xFF55FF,0x55FFFF,0xFFFFFF     // Blue , Magenta, Cyan , White
};

/* ----------------------------------------------------------------------- */
/*
 * Default VGA font, in default_font.cpp.
 *
 * Used when not enough video memory to store the current font.
 */
extern int default_vga_font_width;
extern int default_vga_font_height;
extern unsigned char default_vga_font_data[];

/* ----------------------------------------------------------------------- */

screen_cocon_render::screen_cocon_render( cocon_video_mem_info * info )
    : info_( *info )
    , vram_( (__u8*)info )
    , fb_( 0 )
    , palette_( default_color_palette )
{
    reset( );
}

/* ----------------------------------------------------------------------- */

screen_cocon_render::~screen_cocon_render( )
{
    if ( fb_ )
        delete[] fb_;
}

/* ----------------------------------------------------------------------- */
/*
 * Re-set internal structures.
 */
void screen_cocon_render::reset( )
{
    if ( fb_ )
        delete[] fb_;

    if ( info_.font_count )
    {
        font_data_ = vram_ + info_.font_data;
        fw_        = info_.font_width;
        fh_        = info_.font_height;
    }
    else
    {
        font_data_ = default_vga_font_data;
        fw_        = default_vga_font_width;
        fh_        = default_vga_font_height;
    }

    scr_base_ = (__u16 *) (vram_ + info_.scr_base);
    cur_h_    = fh_ / 4;    /* FIXME: Let the user choose the cursor size */
    w_        = info_.num_cols * fw_;
    h_        = info_.num_rows * fh_;
    fb_       = new __u32[ w_ * h_ ];

    // Re-render screen
    __u16* scr = scr_base_;
    scr -= info_.scrollback*info_.num_cols;
    for ( int j=0; j < info_.num_rows; ++j )
        render_str( j,0, &scr[j*info_.num_cols], info_.num_cols );
}

/* ----------------------------------------------------------------------- */
/*
 * Renders current screen.
 *
 * NOTE: draw() must be called with the video memory locked.
 */
void screen_cocon_render::draw( int x, int y )
{
    /*
     * We need to check the info has not changed since the last reset.
     * If it has, cancel drawing. It's up to the screen widget to re-set us.
     */
    if ( (info_.header.flags & CO_VIDEO_FLAG_INFO_CHANGE)
            || id() != CO_VIDEO_MAGIC_COCON )
        return;

    /*
     * We can be called even if the console buffer hasn't changed, like
     * on minimize/restore, so only re-render screen if it needs to.
     */
    if ( info_.header.flags & CO_VIDEO_FLAG_DIRTY )
    {
        __u16* scr = scr_base_;
        scr -= info_.scrollback*info_.num_cols;
        // Full rendering of the current view
        for ( int j=0; j < info_.num_rows; ++j )
            render_str( j,0, &scr[j*info_.num_cols], info_.num_cols );
        // Clear the dirty flag
        info_.header.flags &= ~CO_VIDEO_FLAG_DIRTY;
    }

    /*
     * Now we can do the real drawing of the rendered screen
     */
    fl_draw_image( (uchar*)fb_, x,y, w_,h_, 4 );

    /*
     * Draw cursor, if visible and not in scrollback mode
     */
    if ( !(info_.header.flags & CO_VIDEO_COCON_CURSOR_HIDE)
                && !info_.scrollback )
    {
        fl_color( FL_WHITE );
        fl_rectf( x + fw_*info_.cur_x, y + fh_*(info_.cur_y+1) - cur_h_ - 1,
                  fw_, cur_h_ );
    }
}

/* ----------------------------------------------------------------------- */
/*
 * Render character/attribute at position (row,col).
 */
void screen_cocon_render::render_char( int row, int col, __u16 chattr )
{
    // Decode char & attributes
    const unsigned ch = chattr & 0xFF;
    const unsigned fg = (chattr >>  8) & 0x0F;
    const unsigned bg = (chattr >> 12) & 0x0F;
    // Font metrics & get pointer to character font data
    const unsigned fbw = (fw_-1)/8 + 1;          // bytes/char scan line
    const __u8* chdata = font_data_ + ch*fh_*fbw; // font data

    /*
     * FIXME:
     *      I'm sure there are a ton of optimized ways of doing this.
     *      Patches are welcome ;-)
     */
    for ( unsigned h=0; h < fh_; ++h )
        for( unsigned b=0; b < fw_; ++b )
        {
            const unsigned cs = chdata[ h*fbw + b/8 ];
            const bool is_set = cs & (1 << (7-(b%8)));
            pixel( c2x(col)+b, r2y(row)+h ) = is_set? color(fg) : color(bg);
        }
}

/* ----------------------------------------------------------------------- */
/*
 * Render string 'str' at position (row,col).
 */
void screen_cocon_render::render_str( int row, int col, __u16 * str, int len )
{
    for ( int i=0; i < len; ++i )
        render_char( row,col+i, str[i] );
}

/* ----------------------------------------------------------------------- */

