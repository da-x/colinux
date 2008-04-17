
/*
 * This source code is a part of coLinux source package.
 *
 * Copyright (C) 2008 Steve Shoecraft <sshoecraft@earthlink.net>
 *
 * The code is licensed under the GPL.  See the COPYING file in
 * the root directory.
 *
 */

#define USE_Q 0

#include "ddk.h"
#include <ddk/ntifs.h>
#include <ddk/ntdddisk.h>
#include <ddk/ntddscsi.h>

#include <colinux/kernel/scsi.h>
#include <colinux/kernel/transfer.h>
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

typedef /*NTOSAPI*/ NTSTATUS DDKAPI (*xfer_func_t)( /*IN*/ HANDLE  FileHandle, /*IN*/ HANDLE  Event  /*OPTIONAL*/, /*IN*/ PIO_APC_ROUTINE  ApcRoutine  /*OPTIONAL*/, /*IN*/ PVOID  ApcContext  /*OPTIONAL*/, /*OUT*/ PIO_STATUS_BLOCK  IoStatusBlock, /*IN*/ PVOID  Buffer, /*IN*/ ULONG  Length, /*IN*/ PLARGE_INTEGER  ByteOffset  /*OPTIONAL*/, /*IN*/ PULONG  Key  /*OPTIONAL*/);

extern PDEVICE_OBJECT coLinux_DeviceObject;

#include <asm/scatterlist.h>

struct _io_req {
	int in_use;
	co_monitor_t *mp;
	co_scsi_dev_t *dp;
	co_scsi_io_t io;
	IO_STATUS_BLOCK isb;
	PIO_WORKITEM pIoWorkItem;
	xfer_func_t func;
#if !COSCSI_ASYNC
	int result;
#endif
};

#if USE_Q
#define IO_QUEUE_SIZE 64
static struct _io_req io_queue[IO_QUEUE_SIZE];
static int next_entry = 0;
#endif

#if COSCSI_DEBUG_OPEN || COSCSI_DEBUG_IO || COSCSI_DEBUG_PASS
static char *iostatus_string(IO_STATUS_BLOCK IoStatusBlock) {

	switch(IoStatusBlock.Information) {
	case FILE_CREATED:
		return "FILE_CREATED";
	case FILE_OPENED:
		return "FILE_OPENED";
	case FILE_OVERWRITTEN:
		return "FILE_OVERWRITTEN";
	case FILE_SUPERSEDED:
		return "FILE_SUPERSEDED";
	case FILE_EXISTS:
		return "FILE_EXISTS";
	case FILE_DOES_NOT_EXIST:
		return "FILE_DOES_NOT_EXIST";
	default:
		return "unknown";
	}
}
#endif

/*
 * Open
*/
int scsi_file_open(co_monitor_t *cmon, co_scsi_dev_t *dp) {
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING unipath;
	ACCESS_MASK DesiredAccess;
	ULONG FileAttributes, CreateDisposition, CreateOptions;
	co_rc_t rc;

#if COSCSI_DEBUG_OPEN
	co_debug("scsi_file_open: pathname: %s", dp->conf->pathname);
#endif

	rc = co_winnt_utf8_to_unicode(dp->conf->pathname, &unipath);
	if (!CO_OK(rc)) return 1;

	DesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE;
	FileAttributes = FILE_ATTRIBUTE_NORMAL;
	CreateDisposition = (dp->conf->is_dev ? FILE_OPEN : FILE_OPEN_IF);
	CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;

	/* Kernel handle needed for IoQueueWorkItem */
	co_debug("init attr");
	InitializeObjectAttributes(&ObjectAttributes, &unipath,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

again:
	co_debug("opening...");
	status = ZwCreateFile((HANDLE *)&dp->os_handle, DesiredAccess, &ObjectAttributes, &IoStatusBlock, 0,
			FileAttributes, 0, CreateDisposition, CreateOptions, NULL, 0);
#if COSCSI_DEBUG_OPEN
	co_debug("ZwCreateFile: status: %x, iostatus: %s", (int)status, iostatus_string(IoStatusBlock));
#endif

	/* If a size is specified, extend/trunc the file */
	if (status == STATUS_SUCCESS && dp->conf->size != 0 && dp->conf->is_dev == 0) {
		FILE_END_OF_FILE_INFORMATION eof;

		eof.EndOfFile.QuadPart = (unsigned long long)dp->conf->size * 1048576LL;
#if COSCSI_DEBUG_OPEN
		co_debug("eof.EndOfFile.QuadPart: %lld", eof.EndOfFile.QuadPart);
#endif
		status = ZwSetInformationFile(dp->os_handle, &IoStatusBlock, &eof, sizeof(eof), FileEndOfFileInformation);
#if COSCSI_DEBUG_OPEN
		co_debug("ZwSetInformationFile: status: %x, iostatus: %s", (int)status, iostatus_string(IoStatusBlock));
#endif
		if (status == STATUS_SUCCESS) {
			co_debug("setting size...");
			dp->conf->size = 0;
			co_debug("closing...");
			ZwClose(dp->os_handle);
			co_debug("opening again...");
			goto again;
		}
	}

#if COSCSI_DEBUG_OPEN
	co_debug("scsi_file_open: os_handle: %p", dp->os_handle);
#endif

	co_winnt_free_unicode(&unipath);

	return (status != STATUS_SUCCESS);
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

/* Async is defined in kernel header! */
#if COSCSI_ASYNC
static void scsi_send_intr(co_monitor_t *cmon, int unit, void *ctx, int rc, int delta) {
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

typedef struct {
	HANDLE file_handle;
	LARGE_INTEGER offset;
	xfer_func_t func;
} scsi_transfer_file_block_data_t;

static co_rc_t scsi_transfer_file_block(co_monitor_t *cmon, 
				  void *host_data, void *linuxvm, 
				  unsigned long size, 
				  co_monitor_transfer_dir_t dir)
{
	IO_STATUS_BLOCK isb;
	NTSTATUS status;
	scsi_transfer_file_block_data_t *data = host_data;

	status = data->func(data->file_handle,
				NULL,
				NULL,
				NULL, 
				&isb,
				linuxvm,
				size,
				&data->offset,
				NULL);

	if (status != STATUS_SUCCESS) {
		co_debug_error("scsi io failed: %p %lx (reason: %x)",
				linuxvm, size, (int)status);
		return co_status_convert(status);
	}

	data->offset.QuadPart += size;

	return CO_RC_OK;
}

#if COSCSI_ASYNC
static VOID DDKAPI _scsi_dio(PDEVICE_OBJECT DeviceObject, PVOID Context) {
#else
static int _scsi_dio(PDEVICE_OBJECT DeviceObject, PVOID Context) {
#endif
	struct _io_req *r = Context;
	struct scatterlist *sg;
	co_pfn_t sg_pfn;
	unsigned char *sg_page;
	PIO_WORKITEM wip;
	int x;
	co_rc_t rc;
	scsi_transfer_file_block_data_t data;
	int bytes_req = 0;

	data.file_handle = (HANDLE)r->dp->os_handle;
	data.offset.QuadPart = r->io.offset;
	data.func = r->func;

	/* Map the SG */
	rc = co_monitor_host_linuxvm_transfer_map(r->mp, r->io.sg,
					r->io.count * sizeof(struct scatterlist),
					(void*)&sg, &sg_page, &sg_pfn);
	if (!CO_OK(rc)) {
		co_debug("scsi_io: error mapping sg");
		goto io_done;
	}

	/* For each vector */
	for(x=0; x < r->io.count; x++, sg++) {
		bytes_req += sg->length;
		rc = co_monitor_host_linuxvm_transfer(
				r->mp,
				&data,
				scsi_transfer_file_block,
				sg->dma_address + CO_ARCH_KERNEL_OFFSET,
				sg->length,
				0);
		if (!CO_OK(rc))	break;
	}

	co_monitor_host_linuxvm_transfer_unmap(r->mp, sg_page, sg_pfn);

io_done:
#if COSCSI_ASYNC
	/* Send interrupt */
	scsi_send_intr(r->mp, r->dp->unit, r->io.scp, (CO_OK(rc) == 0), bytes_req - (int)(data.offset.QuadPart - r->io.offset));
#endif

	/* Free WorkItem */
	wip = r->pIoWorkItem;
#if USE_Q
	r->in_use = 0;
#else
	co_os_free(r);
#endif
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
#if USE_Q
again:
#if CO_DEBUG_IO
	co_debug("next_entry: %d", next_entry);
#endif
        r = &io_queue[next_entry++];
        if (next_entry >= IO_QUEUE_SIZE) next_entry = 0;
        if (r->in_use) goto again;
	r->in_use = 1;
#else
	r = (struct _io_req *) co_os_malloc(sizeof(*r));
	if (!r) return 1;
#endif
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
	IoQueueWorkItem(r->pIoWorkItem, _scsi_dio, CriticalWorkQueue, r);
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
		rc = co_monitor_host_linuxvm_transfer_map(cmon, pass->buffer, pass->buflen,
					(void*)&buffer, &page, &pfn);
		if (!CO_OK(rc)) {
			co_debug("scsi_pass: get_pfn: %x", (int)rc);
			goto err_out;
		}
	} else {
		page = 0;
		buffer = 0;
	}

#if COSCSI_DEBUG_PASS
	co_debug("scsi_pass: handle: %p, buffer: %p, buflen: %ld\n", dp->os_handle, buffer, pass->buflen);
	co_debug("scsi_pass: cdb_len: %d, write: %d\n", pass->cdb_len, pass->write);
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
	co_debug("scsi_pass: ntstatus: %X, ScsiStatus: %d\n", (int)status, (req.Spt.ScsiStatus >> 1));
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
			co_debug("data:\n");
			p = buffer;
			line[0] = 0;
			for(x=y=0; x < pass->buflen; x++) {
				sprintf(temp,"0x%02x, ", p[x]);
				if (strlen(line) + strlen(temp) >= sizeof(line)-1) {
					strcat(line,"\n");
					co_debug(line);
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
	if (page) co_monitor_host_linuxvm_transfer_unmap(cmon, page, pfn);

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
