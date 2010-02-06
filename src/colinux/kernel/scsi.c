
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

	if (unit < CO_MODULE_MAX_COSCSI)
		dp = cmon->scsi_devs[unit];
	else
		return 0;

	if (!dp) return 0;

#if COSCSI_DEBUG
	co_debug("dp: %p", dp);
#endif
	return dp;
}

void co_scsi_request(co_monitor_t *cmon, int op, int unit) {
	struct co_scsi_dev *dp;

#if COSCSI_DEBUG
	co_debug("co_scsi_op: op: %x, unit: %x\n", op, unit);
#endif

 	dp = get_dp(cmon,unit);
	if (!dp && op != CO_SCSI_GET_CONFIG) {
		co_passage_page->params[0] = 1;
		return;
	}

	switch(op) {
	case CO_SCSI_GET_CONFIG: /* 0 */
		{
			register int x;

			for(x=0; x < CO_MODULE_MAX_COSCSI; x++) {
				if (cmon->scsi_devs[x])
					co_passage_page->params[x+1] = cmon->scsi_devs[x]->conf->type | COSCSI_DEVICE_ENABLED;
				else
					co_passage_page->params[x+1] = 0;
			}
			co_passage_page->params[0] = 0;
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
			co_scsi_io_t *iop = (co_scsi_io_t *) &co_passage_page->params[3];

			co_passage_page->params[0] = scsi_file_io(cmon, dp, iop);
		}
		break;
	case CO_SCSI_PASS: /* 5 */
		{
			co_scsi_pass_t *pass = (co_scsi_pass_t *) &co_passage_page->params[3];

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
	int i;

	for (i=0; i < CO_MODULE_MAX_COSCSI; i++) {
		co_scsi_dev_t *dp = mp->scsi_devs[i];
		if (!dp) continue;

#if COSCSI_DEBUG
		co_debug("unit: %d, handle: %p", i, dp->os_handle);
#endif
		if (dp->os_handle) scsi_file_close(mp, dp);

		mp->scsi_devs[i] = NULL;
		co_monitor_free(mp, dp);
	}
}
