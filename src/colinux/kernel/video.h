
#ifndef __CO_KERNEL_VIDEO_H__
#define __CO_KERNEL_VIDEO_H__

/*
 * This source code is a part of coLinux source package.
 *
 * Copyright (C) 2008 Steve Shoecraft <sshoecraft@earthlink.net>
 *
 * The code is licensed under the GPL.  See the COPYING file in
 * the root directory.
 *
 */

/*
 * Video memory
 *
 * video_buffer      : virtual address the video memory buffer
 * video_vm_address  : virtual address of the bufer in the guest (4MB aligned)
 * video_size        : size, in bytes, of the video memory.
 *                      2048x1536x32 = 12M
 *                      1920x1080x32 = 8M
 *                      default=DeskTopSize() from windows and use 8M on unix
 *                      override with parameter videomem=
 * video_user_address: virtual address of the buffer in user_space
 * video_user_handle : handle for the user address mapping
 * video_user_id     : PID of the video client process
*/

struct co_video_dev {
        void *          video_buffer;
        vm_ptr_t        video_vm_address;
        unsigned long   video_size;
        void *          video_user_address;
        void *          video_user_handle;
        co_id_t         video_user_id;
};

#endif
