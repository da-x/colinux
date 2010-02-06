/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#include "screen_cofb.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>


/* ----------------------------------------------------------------------- */

screen_cofb_render::screen_cofb_render( cofb_video_mem_info * info )
    : info_( *info )
    , vram_( (__u8*)info )
{
    reset( );
}

/* ----------------------------------------------------------------------- */

screen_cofb_render::~screen_cofb_render( )
{
}

/* ----------------------------------------------------------------------- */
/*
 * Re-set internal structures.
 */
void screen_cofb_render::reset( )
{
    fb_ = (__u32 *) (vram_ + info_.fb_start);
}

/* ----------------------------------------------------------------------- */
/*
 * Renders current screen.
 *
 * NOTE: draw() must be called with the video memory locked.
 */
void screen_cofb_render::draw( int x, int y )
{
    /*
     * We need to check the info has not changed since the last reset.
     * If it has, cancel drawing. It's up to the screen widget to re-set us.
     */
    if ( (info_.header.flags & CO_VIDEO_FLAG_INFO_CHANGE)
            || id() != CO_VIDEO_MAGIC_COFB )
        return;

    /*
     * Now we can do the real drawing of the rendered screen.
     */
    switch ( info_.bpp )
    {
    case 16:
	/*
	 * This doesn't work, as fl_draw_image() draws grayscale with this,
	 * but at least allows us to see something on this mode (some fb
	 * programs only use this, like fbsplash and qingy).
	 */
        fl_draw_image( (uchar*)fb_, x,y, w(),h(), 2, info_.bwidth );
        break;
    case 24:
        fl_draw_image( (uchar*)fb_, x,y, w(),h(), 3, info_.bwidth );
        break;
    case 32:
        fl_draw_image( (uchar*)fb_, x,y, w(),h(), 4, info_.bwidth );
        break;
    }

    // Finish by clearing the dirty flag
    info_.header.flags &= ~CO_VIDEO_FLAG_DIRTY;
}

/* ----------------------------------------------------------------------- */
