
/*
 * This source code is a part of coLinux source package.
 *
 * Copyright (C) 2008 Steve Shoecraft <sshoecraft@earthlink.net>
 *
 * The code is licensed under the GPL.  See the COPYING file in
 * the root directory.
 *
 */

#include <colinux/common/libc.h>
#include <colinux/common/ioctl.h>
#include <colinux/kernel/monitor.h>
#include <colinux/os/alloc.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/misc.h>
#include <colinux/kernel/video.h>

#include <linux/covideo.h>

#define COVIDEO_DEBUG 1

static int co_video_test(co_monitor_t *cmon, co_video_dev_t *dp) {
        unsigned char *p, *t, *t0, *t1;
        int npages,rc;
        register int i;

	rc = 1;
	t0 = t1 = 0;
	if ((t0 = co_os_malloc(CO_ARCH_PAGE_SIZE)) == 0) goto test_out;
        co_memset(t0, 0, CO_ARCH_PAGE_SIZE);
	if ((t1 = co_os_malloc(CO_ARCH_PAGE_SIZE)) == 0) goto test_out;
        co_memset(t1, 0xFF, CO_ARCH_PAGE_SIZE);
        npages = dp->desc.size >> CO_ARCH_PAGE_SHIFT;

	/* Compare the sent buffer */
        p = (unsigned char*)dp->buffer;
        for(i=0; i < npages; i++) {
                t = (i & 1 ? t1 : t0);
                if (co_memcmp(p, t, CO_ARCH_PAGE_SIZE) != 0)
			goto test_out;
                p += CO_ARCH_PAGE_SIZE;
        }

	/* Send the opposite pattern */
        p = (unsigned char*)dp->buffer;
        for(i=0; i < npages; i++) {
                t = (i & 1 ? t0 : t1);
                co_memcpy(p, t, CO_ARCH_PAGE_SIZE);
                p += CO_ARCH_PAGE_SIZE;
        }
	rc = 0;

test_out:
	co_os_free(t0);
	co_os_free(t1);
        return rc;
}

static co_video_dev_t *get_dp(co_monitor_t *cmon, int unit) {
	co_video_dev_t *dp;

#if COVIDEO_DEBUG
	co_debug("unit: %d", unit);
#endif
	if (unit < 0 || unit > CO_MODULE_MAX_COVIDEO) return 0;
        dp = cmon->video_devs[unit];
#if COVIDEO_DEBUG
	co_debug("dp: %p", dp);
#endif

	return dp;
}

void co_video_request(co_monitor_t *cmon, int op, int unit) {
	co_video_dev_t *dp;

#if COVIDEO_DEBUG
	co_debug("op: %d, unit: %d", op, unit);
#endif

	dp = get_dp(cmon, unit);
	if (!dp) {
		co_passage_page->params[0] = 1;
		return;
	}

	switch(op) {
	case CO_VIDEO_GET_CONFIG:
		{
			//covideo_config_t *cp = (covideo_config_t *) &co_passage_page->params[1];
			unsigned long x = dp->desc.width;
			x = (x<<13) | (0x1fff & dp->desc.height);
			x = (x<<6) | (0x3f & dp->desc.bpp);
			co_passage_page->params[1] = (unsigned long)dp->buffer;
			co_passage_page->params[2] = dp->desc.size;

		/*	cp->buffer = (void*)dp->buffer;
			cp->size = dp->desc.size;
			cp->width = dp->desc.width;
			cp->height = dp->desc.height;
			cp->bpp = dp->desc.bpp;*/
			co_passage_page->params[3] = x;
			co_passage_page->params[0] = 0;
		}
		break;
	case CO_VIDEO_TEST:
		co_passage_page->params[0] = co_video_test(cmon, dp);
		break;
	default:
		co_passage_page->params[0] = 1;
		break;
	}
}

/* Init a single device */
int co_monitor_video_device_init(co_monitor_t *cmon, int unit, co_video_dev_desc_t *cp) {
	co_video_dev_t *dp;
	co_rc_t rc;

#if COVIDEO_DEBUG
	co_debug("unit: %d, size: %d\n", unit, cp->desc.size);
#endif

	rc = co_monitor_malloc(cmon, sizeof(co_video_dev_t), (void **)&dp);
	if (!CO_OK(rc)) return rc;

	co_memset(dp, 0, sizeof(co_video_dev_t *));
	dp->unit = unit;
	dp->desc.size = cp->desc.size;
	cmon->video_devs[unit] = dp;

	dp->buffer = co_os_malloc(dp->desc.size);
	if (!dp->buffer) return CO_RC(OUT_OF_MEMORY);

	return 0;
}

/* Unregister & free all devs */
void co_monitor_unregister_video_devices(co_monitor_t *cmon) {
	co_video_dev_t *dp;
	register int i;

	for(i=0; i < CO_MODULE_MAX_COVIDEO; i++) {
		dp = cmon->video_devs[i];
		if (!dp) continue;

		/* Free the buffer */
#if COVIDEO_DEBUG
		co_debug("unit: %d, buffer: %p", i, dp->buffer);
#endif
		co_monitor_free(cmon, dp->buffer);

		cmon->video_devs[i] = NULL;
		co_monitor_free(cmon, dp);
	}
}

/*
 * Map video buffer into user space.
 */
co_rc_t
co_monitor_user_video_attach(co_monitor_t *cmon, co_monitor_ioctl_video_t *params) {

	struct co_video_dev *dp;
	unsigned int npages;
	co_rc_t rc;

	dp = get_dp(cmon, params->unit);
        if (!dp) {
                // use ZERO to indicate video not supported
                params->address = NULL;
                return CO_RC(OK);
        }

	npages = dp->desc.size >> CO_ARCH_PAGE_SHIFT;

        /* FIXME: Return a CO_RC_ALREADY_ATTACHED like error */
        if ( cmon->video_user_id != CO_INVALID_ID ){
                co_debug_system( "Error video already attached fixme \n");
                return CO_RC(ERROR);
        }

	/* Create user space mapping */
	//rc = co_os_userspace_map(dp->buffer, npages, &params->address, &params->handle);
        rc = co_os_userspace_map( dp->buffer, npages,
                &cmon->video_user_address, &cmon->video_user_handle );
	if ( !CO_OK(rc) ) {
		co_debug_system("Error mapping video into user space! (rc=%x)\n", (unsigned int)rc);
		return rc;
	}

#ifdef COVDEO_DEBUG
        co_debug_system("monitor: video_user_address=%08lXh", (long)cmon->video_user_address );
#endif

        /* Remember which process "owns" the video mapping */
        cmon->video_user_id = co_os_current_id( );

        /* Return user mapped video buffer address */
        params->address = cmon->video_user_address;

        return CO_RC(OK);
}

co_rc_t co_video_detach(co_monitor_t *cmon, co_monitor_ioctl_video_t *params) {
	struct co_video_dev *dp;
	unsigned int npages;

	dp = get_dp(cmon, params->unit);
	if (!dp) return CO_RC(ERROR);

	npages = dp->desc.size >> CO_ARCH_PAGE_SHIFT;
	/* XXX vid size is in meg - this isnt really necessary 
	if ((npages * CO_ARCH_PAGE_SIZE) < dp->size) npages++; */

	//co_os_userspace_unmap(params->address, params->handle, npages);

	return CO_RC(OK);
}
