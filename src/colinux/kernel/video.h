
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
#include <colinux/common/ioctl.h>

struct co_video_dev {
	int unit;
	void * buffer;			/* Page-aligned buffer */
	co_video_desc_t desc;
};

typedef struct co_video_dev co_video_dev_t;

extern void co_video_request(co_monitor_t *, int, int);
extern int co_monitor_video_device_init(co_monitor_t *, int, co_video_dev_desc_t *);
extern void co_monitor_unregister_video_devices(co_monitor_t *);
extern co_rc_t co_monitor_user_video_attach(co_monitor_t *, co_monitor_ioctl_video_t *);
//extern co_rc_t co_video_detach(co_monitor_t *, co_monitor_ioctl_video_t *);

#endif
