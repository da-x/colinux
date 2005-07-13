/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#ifndef COLINUX_USER_CONSOLE_SCREEN_COFB_H
#define COLINUX_USER_CONSOLE_SCREEN_COFB_H

#include "screen.h"
extern "C" {
	#include <linux/cooperative_video.h>
}


/* ----------------------------------------------------------------------- */
/*
 * Screen rendering engine for the cofb driver.
 *
 * Implementation of the screen_render abstract class for the cofb driver.
 */
class screen_cofb_render : public screen_render
{
public:
     screen_cofb_render( cofb_video_mem_info * info );
    ~screen_cofb_render( );

    unsigned long id() const            { return CO_VIDEO_MAGIC_COFB; }
    int w() const                       { return info_.width; }
    int h() const                       { return info_.height; }
    void reset( );
    void draw( int x, int y );

private:
    cofb_video_mem_info     &   info_;
    __u8                    *   vram_;
    __u32                   *   fb_;
};


#endif // COLINUX_USER_CONSOLE_SCREEN_COFB_H
