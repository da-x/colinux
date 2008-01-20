
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
#include <colinux/common/version.h>
#include <colinux/os/alloc.h>
#include <colinux/os/kernel/alloc.h>
#include <linux/cooperative.h>

#include <scsi/coscsi.h>

#define COSCSI_DEBUG 0

#include "scsi.h"
#include "monitor.h"
#include "transfer.h"
#include "pages.h"

static struct co_scsi_dev *get_dp(co_monitor_t *cmon, int unit) {
	struct co_scsi_dev *dp;

#if COSCSI_DEBUG
	co_debug("unit: %d, max: %d\n", unit, CO_MODULE_MAX_COSCSI);
#endif
	if (unit < CO_MODULE_MAX_COSCSI)
		dp = cmon->scsi_devs[unit];
	else
		return 0;

	if (!dp) return 0;

	return dp;
}

void co_scsi_op(co_monitor_t *cmon) {
	struct co_scsi_dev *dp = get_dp(cmon,co_passage_page->params[1]);

#if COSCSI_DEBUG
	co_debug("co_scsi_op: op: %lx, unit: %lx, dp: %p\n",
		co_passage_page->params[0], co_passage_page->params[1], dp);
#endif

	switch(co_passage_page->params[0]) {
	case CO_SCSI_GET_CONFIG: /* 0 */
		{
			register int x;

			for(x=0; x < CO_MODULE_MAX_COSCSI; x++) {
				if (cmon->scsi_devs[x])
					co_passage_page->params[x] =  cmon->scsi_devs[x]->conf->type | COSCSI_DEVICE_ENABLED;
				else
					co_passage_page->params[x] = 0;
			}
		}
		break;
	case CO_SCSI_OPEN: /* 1 */
		{
			co_passage_page->params[0] = scsi_file_open(cmon, dp);
			co_passage_page->params[1] = (unsigned long) dp->os_handle;
		}
		break;
	case CO_SCSI_CLOSE: /* 2 */
		{
			co_passage_page->params[0] = scsi_file_close(cmon, dp);
		}
		break;
	case CO_SCSI_SIZE: /* 3 */
		{
			unsigned long long size;

			co_passage_page->params[0] = scsi_file_size(cmon, dp, &size);
			*((unsigned long long *)&co_passage_page->params[1]) = size;
		}
		break;
	case CO_SCSI_IO: /* 4 */
		{
			co_scsi_io_t *iop;

			iop = (co_scsi_io_t *) &co_passage_page->params[2];
			co_passage_page->params[0] = scsi_file_io(cmon, dp, iop);
		}
		break;
	case CO_SCSI_PASS: /* 5 */
		{
			co_scsi_pass_t *pass = (co_scsi_pass_t *) &co_passage_page->params[2];

			co_passage_page->params[0] = scsi_pass(cmon, dp, pass);
		}
		break;
	default:
		co_debug("co_scsi_op: unknown operation!\n");
		break;
	}
}

void co_monitor_unregister_and_free_scsi_devices(co_monitor_t *mp)
{
	long i;

	co_debug("co_monitor_unregister_and_free_scsi_devices");
	for (i=0; i < CO_MODULE_MAX_COSCSI; i++) {
		co_scsi_dev_t *dp = mp->scsi_devs[i];
		if (!dp) continue;

		if (dp->os_handle) scsi_file_close(mp, dp);

		mp->scsi_devs[i] = NULL;
		co_monitor_free(mp, dp);
	}

	co_debug("co_monitor_unregister_and_free_scsi_devices --> DONE");
}
