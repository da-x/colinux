
/*
 * This source code is a part of coLinux source package.
 *
 * Copyright (C) 2008 Steve Shoecraft <sshoecraft@earthlink.net>
 *
 * The code is licensed under the GPL.  See the COPYING file in
 * the root directory.
 *
 */

#include "ddk.h"
#include <ddk/ntifs.h>
#include <ddk/ntdddisk.h>
#include <ddk/ntddscsi.h>

#include <colinux/kernel/scsi.h>
#include <colinux/kernel/transfer.h>
#include <colinux/kernel/pages.h>
#include <colinux/os/alloc.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/common/libc.h>
#include <scsi/coscsi.h>

#include "fileio.h"

#ifndef OBJ_KERNEL_HANDLE
#define OBJ_KERNEL_HANDLE 0x00000200L
#endif

#define COSCSI_DEBUG_OPEN 0
#define COSCSI_DEBUG_IO 0
#define COSCSI_DEBUG_PASS 0

/*
 * Open
*/
int scsi_file_open(co_monitor_t *cmon, co_scsi_dev_t *dp) {
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING unipath;
	ACCESS_MASK DesiredAccess;
	ULONG OpenOptions;
	co_rc_t rc;

#if COSCSI_DEBUG_OPEN
	co_debug_system("scsi_file_open: pathname: %s", dp->conf->pathname);
#endif

	rc = co_winnt_utf8_to_unicode(dp->conf->pathname, &unipath);
	if (!CO_OK(rc)) return 1;

	DesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA;
	DesiredAccess |= SYNCHRONIZE;

	OpenOptions = FILE_SYNCHRONOUS_IO_NONALERT;
//	OpenOptions |= FILE_NO_INTERMEDIATE_BUFFERING;
//	OpenOptions |= FILE_RANDOM_ACCESS;
//	OpenOptions |= FILE_WRITE_THROUGH;

	/* Kernel handle needed for IoQueueWorkItem */
	InitializeObjectAttributes(&ObjectAttributes, &unipath,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwOpenFile((HANDLE *)&dp->os_handle, DesiredAccess, &ObjectAttributes, &IoStatusBlock, 0, OpenOptions);

	co_winnt_free_unicode(&unipath);

	if (status != STATUS_SUCCESS) {
		co_debug_lvl(filesystem, 5, "error %x ZwOpenFile('%s')", (int)status, dp->conf->pathname);
		return 1;
	}

#if COSCSI_DEBUG_OPEN
	co_debug_system("scsi_file_open: os_handle: %p", dp->os_handle);
#endif

	return 0;
}

/*
 * Close
*/
int scsi_file_close(co_monitor_t *cmon, co_scsi_dev_t *dp)
{
	NTSTATUS status;

	if (!dp->os_handle) return 0;
	status = ZwClose(dp->os_handle);
	if (status == STATUS_SUCCESS) dp->os_handle = 0;

	return (status != STATUS_SUCCESS);
}

#if COSCSI_ASYNC
static void send_intr(co_monitor_t *cmon, int unit, void *ctx, int rc, int delta) {
	struct {
		co_message_t mon;
		co_linux_message_t linux;
		co_scsi_intr_t info;
	} msg;

	msg.mon.from = CO_MODULE_COSCSI0;
	msg.mon.to = CO_MODULE_LINUX;
	msg.mon.priority = CO_PRIORITY_DISCARDABLE;
	msg.mon.type = CO_MESSAGE_TYPE_OTHER;
	msg.mon.size = sizeof(msg) - sizeof(msg.mon);
	msg.linux.device = CO_DEVICE_SCSI;
	msg.linux.unit = unit;
	msg.info.ctx = ctx;
	msg.info.result = rc;
	msg.info.delta = delta;

	co_monitor_message_from_user(cmon, 0, (co_message_t *)&msg);
}
#endif

typedef /*NTOSAPI*/ NTSTATUS DDKAPI (*xfer_func_t)( /*IN*/ HANDLE  FileHandle, /*IN*/ HANDLE  Event  /*OPTIONAL*/, /*IN*/ PIO_APC_ROUTINE  ApcRoutine  /*OPTIONAL*/, /*IN*/ PVOID  ApcContext  /*OPTIONAL*/, /*OUT*/ PIO_STATUS_BLOCK  IoStatusBlock, /*IN*/ PVOID  Buffer, /*IN*/ ULONG  Length, /*IN*/ PLARGE_INTEGER  ByteOffset  /*OPTIONAL*/, /*IN*/ PULONG  Key  /*OPTIONAL*/);
extern PDEVICE_OBJECT coLinux_DeviceObject;

struct page {
	int bogus;
};
#include <asm/scatterlist.h>

struct _io_req {
	co_monitor_t *mp;
	co_scsi_dev_t *dp;
	co_scsi_io_t io;
	PIO_WORKITEM pIoWorkItem;
	xfer_func_t func;
#if !COSCSI_ASYNC
	int result;
#endif
};

#if COSCSI_ASYNC
static VOID DDKAPI _scsi_io(PDEVICE_OBJECT DeviceObject, PVOID Context) {
#else
static int _scsi_io(PDEVICE_OBJECT DeviceObject, PVOID Context) {
#endif
	struct _io_req *r = Context;
	struct scatterlist *sg;
	IO_STATUS_BLOCK isb;
	LARGE_INTEGER Offset;
	NTSTATUS status;
	void *buffer;
	co_pfn_t sg_pfn, pfn;
	unsigned char *sg_page, *page;
	int bytes_req, bytes_xfer;
	PIO_WORKITEM wip;
	vm_ptr_t vaddr;
	int x,len,size;
	co_rc_t rc;

	bytes_req = bytes_xfer = 0;

	/* Map the SG */
	rc = co_monitor_get_pfn(r->mp, r->io.sg, &sg_pfn);
	if (!CO_OK(rc)) {
		co_debug_system("scsi_io: error mapping sg");
		goto io_done;
	}
	sg_page = co_os_map(r->mp->manager, sg_pfn);
	sg = (struct scatterlist *) (sg_page + (r->io.sg & ~CO_ARCH_PAGE_MASK));

	/* For each vector */
	rc = CO_RC(OK);
	Offset.QuadPart = r->io.offset;
	for(x=0; x < r->io.count; x++, sg++) {
		vaddr = sg->dma_address + CO_ARCH_KERNEL_OFFSET;
		size = sg->length;
		bytes_req += size;
		while(size) {
			len = ((vaddr + CO_ARCH_PAGE_SIZE) & CO_ARCH_PAGE_MASK) - vaddr;
			if (len > size) len = size;

			/* Map page */
			rc = co_monitor_get_pfn(r->mp, vaddr, &pfn);
			if (!CO_OK(rc)) {
				co_debug_system("scsi_host_io: error mapping page!\n");
				break;
			}
			page = co_os_map(r->mp->manager, pfn);
			buffer = page + (vaddr & ~CO_ARCH_PAGE_MASK);

			/* Transfer data */
			status = r->func((HANDLE)r->dp->os_handle, NULL, NULL, NULL, &isb, buffer, len, &Offset, NULL);

			/* Unmap page */
			co_os_unmap(r->mp->manager, page, pfn);

			/* Check status */
			if (status != STATUS_SUCCESS) {
				co_debug("ntstatus: %X\n", (int) status);
				rc = CO_RC(ERROR);
				break;
			}

			/* Update counters */
			size -= len;
			vaddr += len;
			Offset.QuadPart += len;
			bytes_xfer += len;
		}
		if (!CO_OK(rc)) break;
	}

	co_os_unmap(r->mp->manager, sg_page, sg_pfn);

io_done:
#if COSCSI_ASYNC
	/* Send interrupt */
	send_intr(r->mp, r->dp->unit, r->io.scp, (CO_OK(rc) == 0), bytes_req - bytes_xfer);
#endif

	/* Free WorkItem */
	wip = r->pIoWorkItem;
	co_os_free(r);
        IoFreeWorkItem(wip);

#if COSCSI_ASYNC == 0
	return (CO_OK(rc) == 0);
#endif
}

/*
 * Read/Write
*/
int scsi_file_io(co_monitor_t *mp, co_scsi_dev_t *dp, co_scsi_io_t *io) {
	struct _io_req *r;

	/* Setup the req */
#if COSCSI_DEBUG_IO
	co_debug("setting up req...\n");
#endif
	r = (struct _io_req *) co_os_malloc(sizeof(*r));
	if (!r) return 1;
	r->mp = mp;
	r->dp = dp;
	co_memcpy(&r->io, io, sizeof(*io));
	r->pIoWorkItem = IoAllocateWorkItem(coLinux_DeviceObject);
	r->func = (io->write ? ZwWriteFile : ZwReadFile);

	/* Submit the req */
#if COSCSI_DEBUG_IO
	co_debug("submitting req...\n");
#endif
#if COSCSI_ASYNC
	IoQueueWorkItem(r->pIoWorkItem, _scsi_io, CriticalWorkQueue, r);
	return 0;
#else
	return _scsi_io(coLinux_DeviceObject, r);
#endif
}

/*
 * Pass-through
*/
#define SENSELEN 24
int scsi_pass(co_monitor_t *cmon, co_scsi_dev_t *dp, co_scsi_pass_t *pass) {
	IO_STATUS_BLOCK iosb;
	struct {
		SCSI_PASS_THROUGH_DIRECT Spt;
		unsigned char SenseBuf[SENSELEN];
	} req;
	void *buffer;
	co_pfn_t pfn;
	unsigned char *page;
	NTSTATUS status;
	co_rc_t rc;

	/* Map buffer page */
	if (pass->buflen) {
		rc = co_monitor_get_pfn(cmon, pass->buffer, &pfn);
		if (!CO_OK(rc)) {
			co_debug_system("scsi_pass: get_pfn: %x", (int)rc);
			goto err_out;
		}
		page = co_os_map(cmon->manager, pfn);
		buffer = page + (pass->buffer & ~CO_ARCH_PAGE_MASK);
	} else {
		page = 0;
		buffer = 0;
	}

#if COSCSI_DEBUG_PASS
	co_debug_system("scsi_pass: handle: %p, buffer: %p, buflen: %d\n",
		dp->os_handle, buffer, pass->buflen);
	co_debug_system("scsi_pass: cdb_len: %d, write: %d\n", pass->cdb_len, pass->write);
#endif

	co_memset(&req, 0, sizeof(req));
	req.Spt.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	req.Spt.ScsiStatus = 0;
	req.Spt.PathId = 0;
	req.Spt.TargetId = 0;
	req.Spt.Lun = 0;
	req.Spt.CdbLength = pass->cdb_len;
	req.Spt.SenseInfoLength = SENSELEN;
	if (pass->buflen)
		req.Spt.DataIn = (pass->write ? SCSI_IOCTL_DATA_OUT : SCSI_IOCTL_DATA_IN);
	else
		req.Spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
	req.Spt.DataTransferLength = pass->buflen;
	req.Spt.TimeOutValue = 2;
	req.Spt.DataBuffer = buffer;
	req.Spt.SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT);
	co_memcpy(&req.Spt.Cdb, pass->cdb, 12);

	status = ZwDeviceIoControlFile((HANDLE)
			dp->os_handle,			/* FileHandle */
			NULL,				/* Event */
			NULL,				/* ApcRoutine */
			NULL,				/* ApcContext */
			&iosb, 				/* IoStatusBlock */
			IOCTL_SCSI_PASS_THROUGH_DIRECT,	/* IoControlCode */
			&req,				/* InputBuffer */
			sizeof(req),			/* InputBufferLength */
			&req,				/* OutputBuffer */
			sizeof(req)			/* OutputBufferLength */
		);

#if COSCSI_DEBUG_PASS
	co_debug_system("scsi_pass: ntstatus: %X, ScsiStatus: %d\n", status, (req.Spt.ScsiStatus >> 1));
#endif

#if 0
	/* Dump page data to console */
	if (status == STATUS_SUCCESS && req.Spt.ScsiStatus == 0 && pass->buflen) {
		unsigned char *p;
		char line[64],temp[8];
		register int x,y;

		switch(pass->cdb[0]) {
		case 0x12:
		case 0x46:
		case 0x1A:
			co_debug_system("data:\n");
			p = buffer;
			line[0] = 0;
			for(x=y=0; x < pass->buflen; x++) {
				sprintf(temp,"0x%02x, ", p[x]);
				if (strlen(line) + strlen(temp) >= sizeof(line)-1) {
					strcat(line,"\n");
					co_debug_system(line);
					line[0] = 0;
				} else
					strcat(line, temp);
			}
		default:
			break;
		}
	}
#endif

	/* Unmap page */
	if (page) co_os_unmap(cmon->manager, page, pfn);

	if (status == STATUS_SUCCESS)
		return req.Spt.ScsiStatus >> 1;

err_out:
	return 1;
}

int scsi_file_size(co_monitor_t *cmon, co_scsi_dev_t *dp, unsigned long long *size) {
	co_rc_t rc;

	rc = co_os_file_get_size(dp->os_handle, size);

	return (CO_OK(rc) == 0);
}
