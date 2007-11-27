
#include <string.h>
#include <colinux/common/libc.h>
#include <colinux/common/version.h>
#include <scsi/scsi.h>
#include <colinux/os/alloc.h>
#include <linux/cooperative.h>

#include "scsi.h"
#include "monitor.h"
#include "printk.h"
#include "transfer.h"

#include "scsi_disk.h"
#include "scsi_cd.h"

#define DEBUG_XFER 0
#define DEBUG_COMM 0
#define DEBUG_SENSE 0
#define DEBUG_WORKER 0

#if DEBUG_XFER || DEBUG_COMM || DEBUG_SENSE || DEBUG_WORKER
#define DUMP_DATA 1
#endif

char scsi_rev[5];
int have_rev = 0;

#define min(a,b) ((a) < (b) ? (a) : (b))

#if DUMP_DATA
static void _dump_data(co_monitor_t *mp, char *str, void *data, int len) {
        unsigned char *p;
        int x,y;

        printk(mp, "%s(%d bytes):\n",str,len);
        p = data;
        for(x=y=0; x < len; x++) {
                printk(mp," %02x", p[x]);
                y++;
                if (y > 15) {
                        printk(mp,"\n");
                        y = 0;
                }
        }
        if (y) printk(mp,"\n");
}
#define dump_data(m,s,a,b) _dump_data(m,s,a,b)
#else
#define dump_data(m,s,a,b) /* noop */
#endif

static int check_condition(co_scsi_dev_t *dp, int key, int asc, int asq) {
	dp->key = key;
	dp->asc = asc;
	dp->asq = asq;
	return CHECK_CONDITION;
}

static __inline__ int scsi_transfer(scsi_worker_t *wp, void *data, int len) {
	int act_len;
	co_rc_t rc;

	if (wp->req.buflen) {
		act_len = min(wp->req.buflen, len);
#if DEBUG_COMM
		dump_data(wp->mp, "transfer", data, min(act_len,16));
#endif
		rc = co_monitor_host_to_linuxvm(wp->mp, data, wp->req.buffer, act_len);
		if (!CO_OK(rc)) return check_condition(wp->dp, UNIT_ATTENTION, DATA_TRANSFER_ERROR, 0);
	}

	return GOOD;
}

static int scsi_inquiry(scsi_worker_t *wp) {
	int x, alloc_len;
	scsi_page_t *inq = wp->dp->rom->inq;

	alloc_len = (wp->req.cdb[3] << 8) + wp->req.cdb[4];

#if DEBUG_INQ
	printk(cmon, "scsi_inq: alloc_len: %d, buflen: %d\n", alloc_len, wp->req.buflen);
#endif

	/* EVPD? */
	if (wp->req.cdb[1] & 1) {
		for(x=1; inq[x].page; x++) {
			if (wp->req.cdb[2] == inq[x].num)
				return scsi_transfer(wp, inq[x].page, min(alloc_len, inq[x].size));
		}
		return check_condition(wp->dp, ILLEGAL_REQUEST, INVALID_FIELD_IN_CDB, 0);
	} else {
		/* Standard page */
		strcpy((char *)&inq[0].page[8], "coLinux");
		inq[0].page[1] = ((wp->dp->flags & SCSI_DF_RMB) ? 0x80 : 0);
		memcpy(&inq[0].page[16], wp->dp->rom->name, strlen(wp->dp->rom->name)+1);
		memcpy(&inq[0].page[32], scsi_rev, min(4, strlen(scsi_rev)+1));
		return scsi_transfer(wp, inq[0].page, min(alloc_len, inq[0].size));
	}
}

static int read_capacity(scsi_worker_t *wp) {
	if (wp->dp->max_lba > 0xfffffffe || wp->req.cdb[8] & 1) {
		wp->msg[0] = 0xff;
		wp->msg[1] = 0xff;
		wp->msg[2] = 0xff;
		wp->msg[3] = 0xff;
	} else {
		wp->msg[0] = (wp->dp->max_lba >> 24);
		wp->msg[1] = (wp->dp->max_lba >> 16) & 0xff;
		wp->msg[2] = (wp->dp->max_lba >> 8) & 0xff;
		wp->msg[3] = wp->dp->max_lba & 0xff;
	}
	wp->msg[4] = (wp->dp->block_size >> 24);
	wp->msg[5] = (wp->dp->block_size >> 16) & 0xff;
	wp->msg[6] = (wp->dp->block_size >> 8) & 0xff;
	wp->msg[7] = wp->dp->block_size & 0xff;

	return scsi_transfer(wp, &wp->msg, 8);
}

static int mode_sense(scsi_worker_t *wp) {
	unsigned char *ap;
	int offset, bd_len, page;
	scsi_page_t *pages = wp->dp->rom->mode;
	register int x;

	memset(wp->msg, 0, 0x12);
	offset = 4;
	bd_len = 8;

	wp->msg[2] = 0x10; /* DPOFUA */
	wp->msg[3] = bd_len;

	ap = wp->msg + offset;
	if (wp->dp->max_lba > 0xfffffffe) {
		ap[0] = 0xff;
		ap[1] = 0xff;
		ap[2] = 0xff;
		ap[3] = 0xff;
	} else {
		ap[0] = (wp->dp->max_lba >> 24) & 0xff;
		ap[1] = (wp->dp->max_lba >> 16) & 0xff;
		ap[2] = (wp->dp->max_lba >> 8) & 0xff;
		ap[3] = wp->dp->max_lba & 0xff;
	}
	ap[6] = (wp->dp->block_size >> 8) & 0xff;
	ap[7] = wp->dp->block_size & 0xff;

	offset += bd_len;
	ap = wp->msg + offset;
#if DEBUG_SENSE
	printk(wp->mp,"mode_sense: ap: %p, offset: %d\n", ap, offset);
#endif
	page = wp->req.cdb[2] & 0x3f;
	if (page == 0x3f) {
		/* All pages */
		if (wp->req.cdb[3] == 0 || wp->req.cdb[3] == 0xFF) {
			for(x=0; pages[x].page; x++) {
#if DEBUG_SENSE
				dump_data(wp->mp, "page", pages[x].page, pages[x].size);
#endif
				memcpy(ap, pages[x].page, pages[x].size);
				ap += pages[x].size;
				offset += pages[x].size;
			}
		} else {
			return check_condition(wp->dp, ILLEGAL_REQUEST, INVALID_FIELD_IN_CDB, 0);
		}
	} else {
		/* Specific page */
		int found = 0;
		for(x=0; pages[x].page; x++) {
#if DEBUG_SENSE
			printk(wp->mp, "mode_sense: pages[%d].num: %d, page: %d\n", x, pages[x].num, page);
#endif
			if (pages[x].num == page) {
#if DEBUG_SENSE
				dump_data(wp->mp, "page", pages[x].page, pages[x].size);
#endif
				memcpy(ap, pages[x].page, pages[x].size);
				offset += pages[x].size;
				found = 1;
				break;
			}
		}
		if (!found) return check_condition(wp->dp, ILLEGAL_REQUEST, INVALID_FIELD_IN_CDB, 0);
	}
#if DEBUG_SENSE
	printk(wp->mp, "scsi_mode_sense: offset: %d\n", offset);
#endif
	wp->msg[0] = offset - 1;
#if DEBUG_SENSE
	dump_data(wp->mp, "wp->msg", &wp->msg, min(wp->req.cdb[4], offset));
#endif
	return scsi_transfer(wp, wp->msg, min(wp->req.cdb[4], offset));
}

static int scsi_rw(scsi_worker_t *wp) {
	unsigned long long lba;
	unsigned long num;
	unsigned char *cmd = wp->req.cdb;

	if ((*cmd) == READ_16 || (*cmd) == WRITE_16) {
		register int k;

		for (lba = 0, k = 0; k < 8; k++) {
			if (k > 0) lba <<= 8;
			lba += cmd[2 + k];
		}
		num = cmd[13] + (cmd[12] << 8) + (cmd[11] << 16) + (cmd[10] << 24);
	} else if ((*cmd) == READ_12 || (*cmd) == WRITE_12) {
		lba = cmd[5] + (cmd[4] << 8) + (cmd[3] << 16) + (cmd[2] << 24);
		num = cmd[9] + (cmd[8] << 8) + (cmd[7] << 16) + (cmd[6] << 24);
	} else if ((*cmd) == READ_10 || (*cmd) == WRITE_10) {
		lba = (cmd[2] << 24) + (cmd[3] << 16) + (cmd[4] << 8) + cmd[5];
		num = (cmd[7] << 8) + cmd[8];
	} else {
		lba = ((cmd[1] & 0x1f) << 16) + (cmd[2] << 8) + cmd[3];
		num = (cmd[4] == 0) ? 256 : cmd[4];
	}

#if DEBUG_XFER
	printk(wp->mp, "scsi_rw: lba: %lld\n", lba);
	printk(wp->mp, "scsi_rw: num: %ld\n",  num);
#endif

	if (scsi_file_rw(wp, lba, num, cmd[0] & 2))
		return check_condition(wp->dp, HARDWARE_ERROR, 0x3e, 1);
	else
		return GOOD;
}

static void send_intr(co_monitor_t *mp, void *linux_scp, int rc) {
	struct {
		co_message_t mon;
		co_linux_message_t linux;
		char data[sizeof(int)];
	} msg;
	int *result;

	msg.mon.from = CO_MODULE_COSCSI0;
	msg.mon.to = CO_MODULE_LINUX;
	msg.mon.priority = CO_PRIORITY_DISCARDABLE;
	msg.mon.type = CO_MESSAGE_TYPE_OTHER;
	msg.mon.size = sizeof(msg) - sizeof(msg.mon);
	msg.linux.device = CO_DEVICE_SCSI;
	msg.linux.unit = (int) linux_scp;
	msg.linux.size = sizeof(int);
	result = (int *) &msg.linux.data;
	*result = rc;

	co_monitor_message_from_user(mp, 0, (co_message_t *)&msg);
}

void scsi_worker(scsi_worker_t *wp) {
#if DEBUG_WORKER
	printk(wp->mp, "scsi_worker: called!\n");
#endif


	/* Process command */
#if DEBUG_COMM
	dump_data(wp->mp, "cdb", &wp->req.cdb, sizeof(wp->req.cdb));
#endif
	switch(wp->req.cdb[0]) {
	case INQUIRY:
		wp->req.rc = scsi_inquiry(wp);
		break;
	case TEST_UNIT_READY:
		wp->req.rc = GOOD;
		break;
	case REQUEST_SENSE:
		if (wp->req.cdb[1] & 1) {
			wp->msg[0] = 0x72;
			wp->msg[1] = wp->dp->key;
			wp->msg[2] = wp->dp->asc;
			wp->msg[3] = wp->dp->asq;
		} else {
			wp->msg[0] = 0x70;
			wp->msg[2] = wp->dp->key;
			wp->msg[7] = 0xa;
			wp->msg[12] = wp->dp->asc;
			wp->msg[13] = wp->dp->asq;
		}
		wp->req.rc = scsi_transfer(wp, wp->msg, min(wp->req.cdb[4], 18));
		break;
	case READ_CAPACITY:
		wp->req.rc = read_capacity(wp);
		break;
	case REPORT_LUNS:
		memset(wp->msg, 0, 8);
		wp->req.rc = scsi_transfer(wp, wp->msg, 8);
		break;
	case MODE_SENSE:
		wp->req.rc = mode_sense(wp);
		break;
	case READ_6:
	case READ_10:
	case READ_16:
	case WRITE_6:
	case WRITE_10:
	case WRITE_16:
		wp->req.rc = scsi_rw(wp);
		break;
	case SYNCHRONIZE_CACHE:
		wp->req.rc = GOOD;
		break;
	default:
		wp->req.rc = check_condition(wp->dp, ILLEGAL_REQUEST, INVALID_FIELD_IN_CDB, 0);
		break;
	}

#if DEBUG_COMM
	printk(wp->mp, "scsi_worker: srp.rc: %02x (code: %x)\n", wp->req.rc, wp->req.rc & 0xffff);
#endif

	send_intr(wp->mp, wp->req.linux_scp, wp->req.rc);
	co_os_free(wp);
	return;
}

co_rc_t co_monitor_scsi_request(co_monitor_t *mp, unsigned int index, co_scsi_request_t *srp)
{
	co_scsi_dev_t *dp;
	scsi_worker_t *wp;

#ifdef DEBUG_REQ
	printk(mp, "scsi_req: mp: %p, index: %d, srp: %p\n", mp, index, srp);
#endif

	if (index >= CO_MODULE_MAX_COSCSI) {
		co_debug_error("scsi_req: invalid index value!\n");
		return CO_RC(ERROR);
	}

	dp = mp->scsi_devs[index];
#if DEBUG_REQ
	printk(mp, "scsi_req: dp: %p\n", dp);
#endif
	if (!dp) {
		if (srp->cdb[0] == INQUIRY) {
			char temp[96];
			memset(temp,0,sizeof(temp));
			temp[0] = 0x7f;
			temp[3] = 2;
			temp[4] = 92;
			co_monitor_host_to_linuxvm(mp, temp, srp->buffer, srp->cdb[4]);
			send_intr(mp, srp->linux_scp, GOOD);
		} else {
			srp->rc = (DID_ERROR << 16);
		}
		goto req_out;
	}
	dp->mp = mp;

	/* Need to open file before worker thread - otherwise it stays locked even after thread exits */
	if (!dp->os_handle) {
		if (scsi_file_open(dp)) {
			switch(dp->conf->type) {
			case SCSI_PTYPE_CDDVD:
			case SCSI_PTYPE_TAPE:
				srp->rc = check_condition(dp, NOT_READY, MEDIUM_NOT_PRESENT, 0x2);
				break;
			default:
				srp->rc = check_condition(dp, NOT_READY, LOGICAL_UNIT_NOT_READY, 0x2);
				break;
			}
			send_intr(mp, srp->linux_scp, srp->rc);
			goto req_out;
		}
		dp->max_lba = dp->size >> dp->bs_bits;
	}
#if DEBUG_REQ
	printk(mp, "os_handle: %p\n", dp->os_handle);
#endif

	wp = co_os_malloc(sizeof(*wp));
	if (!wp) {
		printk(mp, "scsi_req: co_os_malloc failed!\n");
		srp->rc = (DID_ERROR << 16);
		return CO_RC(OUT_OF_MEMORY);
	}
	wp->mp = mp;
	wp->dp = dp;
	memcpy(&wp->req, srp, sizeof(wp->req));

	/* Queue up the worker */
	scsi_queue_worker(wp,scsi_worker);

req_out:
	return CO_RC(OK);
}

/* User mode routine to init the device struct */
co_rc_t co_monitor_scsi_dev_init(co_scsi_dev_t *dp, int unit, co_scsi_dev_desc_t *conf) {

	/* zero the struct */
	memset(dp, 0, sizeof(*dp));

	if (!have_rev) {
		register char *p;
		int i;

		for(i = 0, p = COLINUX_VERSION; *p; p++) {
			if (*p == '.') continue;
			scsi_rev[i++] = *p;
		}
		scsi_rev[i] = 0;
		have_rev = 1;
		co_debug("scsi_rev: %s\n", scsi_rev);
	}

	dp->conf = conf;
	switch(conf->type) {
	case SCSI_PTYPE_DISK:
		dp->rom = &disk_rom;
		dp->block_size = 512;
		dp->bs_bits = 9;
		break;
	case SCSI_PTYPE_CDDVD:
		dp->rom = &cd_rom;
		dp->block_size = 2048;
		dp->bs_bits = 11;
		dp->flags |= SCSI_DF_RMB;
		break;
#if 0
	case SCSI_PTYPE_TAPE:
		dp->block_size = 8192;
		dp->bs_bits = 13;
		break;
#endif
	default:
		break;
	}

	return CO_RC(OK);
}

void co_monitor_unregister_and_free_scsi_devices(co_monitor_t *mp)
{
	long i;

	for (i=0; i < CO_MODULE_MAX_COSCSI; i++) {
		co_scsi_dev_t *dp = mp->scsi_devs[i];
		if (!dp) continue;

		if (dp->os_handle) scsi_file_close(dp);

		mp->scsi_devs[i] = NULL;
		co_monitor_free(mp, dp);
	}
}
