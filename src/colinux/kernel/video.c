
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
#include <colinux/kernel/monitor.h>
#include <colinux/os/alloc.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/kernel/video.h>

#ifdef CONFIG_COOPERATIVE_VIDEO
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
        npages = dp->size >> CO_ARCH_PAGE_SHIFT;

	/* Compare the sent buffer */
        p = dp->buffer;
        for(i=0; i < npages; i++) {
                t = (i & 1 ? t1 : t0);
                if (co_memcmp(p, t, CO_ARCH_PAGE_SIZE) != 0)
			goto test_out;
                p += CO_ARCH_PAGE_SIZE;
        }

	/* Send the opposite pattern */
        p = dp->buffer;
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

static struct co_video_dev *get_dp(co_monitor_t *cmon, int unit) {
	struct co_video_dev *dp;

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
			covideo_config_t *cp = (covideo_config_t *) &co_passage_page->params[1];

			cp->buffer = dp->buffer;
			cp->size = dp->size;
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
	co_debug("unit: %d, size: %d\n", unit, cp->size);
#endif

	rc = co_monitor_malloc(cmon, sizeof(co_video_dev_t), (void **)&dp);
	if (!CO_OK(rc)) return rc;

	co_memset(dp, 0, sizeof(co_video_dev_t *));
	dp->unit = unit;
	dp->size = cp->size;
	cmon->video_devs[unit] = dp;

	dp->buffer = co_os_malloc(dp->size);
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

co_rc_t co_video_attach(co_monitor_t *cmon, co_monitor_ioctl_video_t *params) {
	struct co_video_dev *dp;
	unsigned int npages;
	co_rc_t rc;

	dp = get_dp(cmon, params->unit);
	if (!dp) return CO_RC(ERROR);

	npages = dp->size >> CO_ARCH_PAGE_SHIFT;
	/* XXX vid size is in meg - this isnt really necessary
	if ((npages * CO_ARCH_PAGE_SIZE) < dp->size) npages++; */

	rc = co_os_userspace_map(dp->buffer, npages, &params->address, &params->handle);
	if ( !CO_OK(rc) ) {
		co_debug_system("Error mapping video into user space! (rc=%x)\n", (int)rc);
		return rc;
	}

#ifdef COVDEO_DEBUG
	co_debug("address: %08lXh\n", params->address );
#endif

        return CO_RC(OK);
}

co_rc_t co_video_detach(co_monitor_t *cmon, co_monitor_ioctl_video_t *params) {
	struct co_video_dev *dp;
	unsigned int npages;

	dp = get_dp(cmon, params->unit);
	if (!dp) return CO_RC(ERROR);

	npages = dp->size >> CO_ARCH_PAGE_SHIFT;
	/* XXX vid size is in meg - this isnt really necessary
	if ((npages * CO_ARCH_PAGE_SIZE) < dp->size) npages++; */

	co_os_userspace_unmap(params->address, params->handle, npages);

	return CO_RC(OK);
}
#endif /* CONFIG_COOPERATIVE_VIDEO */
