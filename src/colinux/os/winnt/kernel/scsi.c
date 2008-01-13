
#include "ddk.h"
#include <ddk/ntifs.h>
#include <ddk/ntdddisk.h>

#include <colinux/kernel/scsi.h>
#include <colinux/common/scsi_types.h>
#include <colinux/kernel/transfer.h>
#include <colinux/kernel/pages.h>
#include <colinux/os/alloc.h>
#include <colinux/os/kernel/alloc.h>
#include <scsi/coscsi.h>

#ifndef OBJ_KERNEL_HANDLE
#define OBJ_KERNEL_HANDLE 0x00000200L
#endif

#define COSCSI_DEBUG_OPEN 0
#define COSCSI_DEBUG_IO 0
#define COSCSI_DEBUG_SIZE 0

extern co_rc_t co_winnt_utf8_to_unicode(const char *src, UNICODE_STRING *unicode_str);
extern void co_winnt_free_unicode(UNICODE_STRING *unicode_str);

static co_rc_t status_convert(NTSTATUS status)
{
	switch (status) {
	case STATUS_SUCCESS: return CO_RC(OK);
	case STATUS_NO_SUCH_FILE:
	case STATUS_OBJECT_NAME_NOT_FOUND: return CO_RC(NOT_FOUND);
	case STATUS_CANNOT_DELETE:
	case STATUS_ACCESS_DENIED: return CO_RC(ACCESS_DENIED);
	case STATUS_INVALID_PARAMETER: return CO_RC(INVALID_PARAMETER);
	default: return CO_RC(ERROR);
	}
}

static co_rc_t scsi_file_detect_size(HANDLE handle, unsigned long long *out_size);

int scsi_file_open(co_scsi_dev_t *dp) {
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

int scsi_file_close(co_scsi_dev_t *dp)
{
	NTSTATUS status;

	if (!dp) return 1;
	if (!dp->os_handle) return 0;
	status = ZwClose(dp->os_handle);
	dp->os_handle = 0;

	return (status != STATUS_SUCCESS);
}

/*
 * Vectored read/write
*/
struct _req {
	co_scsi_dev_t *dp;
	co_scsi_io_t *iop;
	PIO_WORKITEM pIoWorkItem;
	/*NTOSAPI*/ NTSTATUS DDKAPI (*func)( /*IN*/ HANDLE  FileHandle, /*IN*/ HANDLE  Event  /*OPTIONAL*/, /*IN*/ PIO_APC_ROUTINE  ApcRoutine  /*OPTIONAL*/, /*IN*/ PVOID  ApcContext  /*OPTIONAL*/, /*OUT*/ PIO_STATUS_BLOCK  IoStatusBlock, /*IN*/ PVOID  Buffer, /*IN*/ ULONG  Length, /*IN*/ PLARGE_INTEGER  ByteOffset  /*OPTIONAL*/, /*IN*/ PULONG  Key  /*OPTIONAL*/);
};

static void send_intr(co_monitor_t *cmon, void *ctx, int rc) {
	struct {
		co_message_t mon;
		co_linux_message_t linux;
		int result;
	} msg;

	msg.mon.from = CO_MODULE_COSCSI0;
	msg.mon.to = CO_MODULE_LINUX;
	msg.mon.priority = CO_PRIORITY_DISCARDABLE;
	msg.mon.type = CO_MESSAGE_TYPE_OTHER;
	msg.mon.size = sizeof(msg) - sizeof(msg.mon);
	msg.linux.device = CO_DEVICE_SCSI;
	msg.linux.unit = (int) ctx;
	msg.linux.size = sizeof(msg.result);
	msg.result = rc;

	co_monitor_message_from_user(cmon, 0, (co_message_t *)&msg);
}

static VOID DDKAPI _scsi_io(PDEVICE_OBJECT DeviceObject, PVOID Context) {
	struct _req *r = Context;
	IO_STATUS_BLOCK isb;
	LARGE_INTEGER Offset;
	NTSTATUS status;
	void *buffer;
	co_pfn_t pfn;
	unsigned char *page;
	PIO_WORKITEM wip;
	register int x;
	co_rc_t rc;

	/* For each vector */
	rc = CO_RC(OK);
	for(x=0; x < r->iop->hdr.count; x++) {
		/* Map page */
                rc = co_monitor_get_pfn(r->dp->mp, r->iop->vec[x].buffer, &pfn);
                if (!CO_OK(rc)) goto io_done;
                page = co_os_map(r->dp->mp->manager, pfn);
		buffer = page + (r->iop->vec[x].buffer & ~CO_ARCH_PAGE_MASK);

		/* Transfer data */
		Offset.QuadPart = r->iop->vec[x].offset;
		status = r->func((HANDLE)r->dp->os_handle, NULL, NULL, NULL, &isb, buffer, r->iop->vec[x].size, &Offset, NULL);
		if (status != STATUS_SUCCESS) rc = status_convert(status);

		/* Unmap page */
                co_os_unmap(r->dp->mp->manager, page, pfn);

		if (!CO_OK(rc)) break;
	}

io_done:
	/* Send interrupt */
	send_intr(r->dp->mp, r->iop->hdr.ctx, (CO_OK(rc) == 0));

	/* Free WorkItem */
	wip = r->pIoWorkItem;
	co_os_free(r);
        IoFreeWorkItem(wip);
}

extern PDEVICE_OBJECT coLinux_DeviceObject;

int scsi_file_io(co_scsi_dev_t *dp, co_scsi_io_t *iop) {
	struct _req *r;
	register int x;

#if COSCSI_DEBUG_IO
	co_debug_system("scsi_file_io: ctx: %p", iop->hdr.ctx);
	co_debug_system("scsi_file_io: count: %d", iop->hdr.count);
	co_debug_system("scsi_file_io: write: %d", iop->hdr.write);
	co_debug_system("scsi_file_io: vectors:");
#endif

	r = (struct _req *) co_os_malloc(sizeof(*r));
	if (!r) return 1;
	r->dp = dp;
	r->iop = co_os_malloc(sizeof(co_scsi_io_header_t) + (sizeof(co_scsi_io_vec_t)*iop->hdr.count));
	r->pIoWorkItem = IoAllocateWorkItem(coLinux_DeviceObject);
	r->func = (iop->hdr.write ? ZwWriteFile : ZwReadFile);

	/* Make a copy of the iop */
	r->iop->hdr.ctx = iop->hdr.ctx;
	r->iop->hdr.count = iop->hdr.count;
	r->iop->hdr.write = iop->hdr.write;
	for(x=0; x < iop->hdr.count; x++) {
#if COSCSI_DEBUG_IO
		co_debug_system("scsi_file_io: vec[%d].buffer: %x", x, iop->vec[x].buffer);
		co_debug_system("scsi_file_io: vec[%d].offset: %lld", x, iop->vec[x].offset);
		co_debug_system("scsi_file_io: vec[%d].size: %ld", x, iop->vec[x].size);
#endif
		r->iop->vec[x].buffer = iop->vec[x].buffer;
		r->iop->vec[x].offset = iop->vec[x].offset;
		r->iop->vec[x].size = iop->vec[x].size;
	}

	/* Kick off the work item */
	IoQueueWorkItem(r->pIoWorkItem, _scsi_io, DelayedWorkQueue, r);
	return 0;
}

/*****************************************************************************************************
 *
 *
 * 
 *
 *
 *****************************************************************************************************/

static bool_t probe_area(HANDLE handle, LARGE_INTEGER offset, char *test_buffer, unsigned long size)
{
	IO_STATUS_BLOCK isb;
	NTSTATUS status;

	status = ZwReadFile(handle, NULL, NULL,
			    NULL, &isb, test_buffer, size,
			    &offset, NULL);

	if (status != STATUS_SUCCESS)
		return PFALSE;

	return PTRUE;
}

static co_rc_t scsi_file_detect_size_binary_search(HANDLE handle, unsigned long long *out_size)
{
	/*
	 * Binary search the size of the device.
	 *
	 * Yep, it's ugly.
	 *
	 * I haven't found a more reliable way. I thought about switching between
	 * IOCTL_DISK_GET_DRIVE_GEOMETRY, IOCTL_DISK_GET_PARTITION_INFORMATION,
	 * and even IOCTL_CDROM_GET_DRIVE_GEOMETRY depending on the device's type,
	 * but I'm not sure if would work in all cases.
	 *
	 * This *would* work in all cases.
	 */

	LARGE_INTEGER scan_bit;
	LARGE_INTEGER build_size;
	unsigned long test_buffer_size = 0x100;
	unsigned long test_buffer_size_max = 0x4000;
	char *test_buffer;

	test_buffer = co_os_malloc(test_buffer_size_max);
	if (!test_buffer)
		return CO_RC(OUT_OF_MEMORY);

	build_size.QuadPart = 0;

	while (test_buffer_size <= test_buffer_size_max) {
		if (probe_area(handle, build_size, test_buffer, test_buffer_size))
			break;

		test_buffer_size <<= 1;
	}

	if (test_buffer_size >= test_buffer_size_max) {
		co_debug_error("size is zero");
		*out_size = 0;
		co_os_free(test_buffer);
		return CO_RC(ERROR);
	}

	scan_bit.QuadPart = 1;

	/*
	 * Find the smallest invalid power of 2.
	 */
	while (scan_bit.QuadPart != 0) {
		if (!probe_area(handle, scan_bit, test_buffer, test_buffer_size))
			break;
		scan_bit.QuadPart <<= 1;
	}

	if (scan_bit.QuadPart == 0)
		return CO_RC(ERROR);

	while (scan_bit.QuadPart) {
		LARGE_INTEGER with_bit;

		scan_bit.QuadPart >>= 1;
		with_bit.QuadPart = build_size.QuadPart | scan_bit.QuadPart;
		if (probe_area(handle, with_bit, test_buffer, test_buffer_size))
			build_size = with_bit;
	}

	build_size.QuadPart += test_buffer_size;

	*out_size = build_size.QuadPart;

	co_os_free(test_buffer);
	return CO_RC(OK);
}

static co_rc_t scsi_file_detect_size_harddisk(HANDLE handle, unsigned long long *out_size)
{
	GET_LENGTH_INFORMATION length;
	IO_STATUS_BLOCK block;
	NTSTATUS status;

	length.Length.QuadPart = 0;

	status = ZwDeviceIoControlFile(handle, NULL, NULL, NULL, &block,
				       IOCTL_DISK_GET_LENGTH_INFO,
				       NULL, 0, &length, sizeof(length));

	if (status == STATUS_SUCCESS) {
		co_debug("IOCTL_DISK_GET_LENGTH_INFO returned success");
		*out_size = length.Length.QuadPart;
		return CO_RC(OK);
	}

	return CO_RC(ERROR);
}

static co_rc_t scsi_file_detect_size(HANDLE handle, unsigned long long *out_size)
{
	co_rc_t rc;

	rc = scsi_file_detect_size_harddisk(handle, out_size);
	if (CO_OK(rc)) return rc;

	/*
	 * Fall back to binary search.
	 */
	rc = scsi_file_detect_size_binary_search(handle, out_size);
	if (CO_OK(rc)) return rc;

	*out_size = 0;
	return CO_RC(ERROR);
}

int scsi_file_size(co_scsi_dev_t *dp, unsigned long long *size) {
	FILE_STANDARD_INFORMATION fsi;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS status;
	co_rc_t rc;

	rc = CO_RC(OK);
	*size = 0;

	status = ZwQueryInformationFile(dp->os_handle,
					&IoStatusBlock,
					&fsi,
					sizeof(fsi),
					FileStandardInformation);

	if (status == STATUS_SUCCESS)
		*size = fsi.EndOfFile.QuadPart;
	else
		rc = scsi_file_detect_size(dp->os_handle, size);

#if COSCSI_DEBUG_SIZE
	co_debug_system("scsi_file_size: detected size: %llu KBs", (*size >> 10));
#endif
	return (CO_OK(rc) == 0);
}
