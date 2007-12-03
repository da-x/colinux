
#include <colinux/common/libc.h>
#include <colinux/common/version.h>
#include <scsi/scsi.h>
#include <colinux/os/alloc.h>
#include <colinux/os/kernel/alloc.h>
#include <linux/cooperative.h>

#include "scsi.h"
#include "monitor.h"
#include "printk.h"
#include "transfer.h"
#include "pages.h"

void co_scsi_op(co_monitor_t *cmon) {
	co_scsi_dev_t *dp;

	co_debug("co_scsi_op: op: %lx", co_passage_page->params[0]);
	switch(co_passage_page->params[0]) {
	case CO_SCSI_GET_CONFIG:
		{
			register int x;

			for(x=0; x < CO_MODULE_MAX_COSCSI; x++) {
				if (cmon->scsi_devs[x])
					co_passage_page->params[x] = cmon->scsi_devs[x]->conf->type | 0x80;
				else
					co_passage_page->params[x] = 0;
			}
		}
		break;
	case CO_SCSI_OPEN:
		{
			dp = cmon->scsi_devs[co_passage_page->params[1]];
			dp->mp = cmon;
			co_passage_page->params[0] = scsi_file_open(dp);
			co_passage_page->params[1] = (unsigned long) dp->os_handle;
		}
		break;
	case CO_SCSI_CLOSE:
		{
			dp = cmon->scsi_devs[co_passage_page->params[1]];
			dp->mp = cmon;
			co_passage_page->params[0] = scsi_file_close(dp);
			dp->os_handle = 0;
		}
		break;
	case CO_SCSI_IO:
		{
			co_scsi_io_t *iop;

			dp = cmon->scsi_devs[co_passage_page->params[1]];
			dp->mp = cmon; 
			iop = (co_scsi_io_t *) &co_passage_page->params[2];
			co_passage_page->params[0] = scsi_file_io(dp, iop);
		}
		break;
	case CO_SCSI_SIZE:
		{
			unsigned long long size;

			dp = cmon->scsi_devs[co_passage_page->params[1]];
			dp->mp = cmon;
			co_passage_page->params[0] = scsi_file_size(dp, &size);
			*((unsigned long long *)&co_passage_page->params[1]) = size;
		}
		break;
	default:
		co_debug("co_scsi_op: unknown operation!");
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

		if (dp->os_handle) scsi_file_close(dp);

		mp->scsi_devs[i] = NULL;
		co_monitor_free(mp, dp);
	}

	co_debug("co_monitor_unregister_and_free_scsi_devices --> DONE");
}
