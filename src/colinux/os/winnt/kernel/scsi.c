
#include "ddk.h"
#include <ddk/ntifs.h>
#include <ddk/ntdddisk.h>

#include <colinux/kernel/scsi.h>
#include <colinux/common/scsi_types.h>
#include <colinux/kernel/transfer.h>
#include <colinux/kernel/printk.h>
#include <colinux/os/alloc.h>

#ifndef OBJ_KERNEL_HANDLE
#define OBJ_KERNEL_HANDLE 0x00000200L
#endif

#define DEBUG_OPEN 0
#define DEBUG_XFER 0

extern co_rc_t co_winnt_utf8_to_unicode(const char *src, UNICODE_STRING *unicode_str);
extern void co_winnt_free_unicode(UNICODE_STRING *unicode_str);

static co_rc_t status_convert(NTSTATUS status)
{
	switch (status) {
	case STATUS_NO_SUCH_FILE:
	case STATUS_OBJECT_NAME_NOT_FOUND: return CO_RC(NOT_FOUND);
	case STATUS_CANNOT_DELETE:
	case STATUS_ACCESS_DENIED: return CO_RC(ACCESS_DENIED);
	case STATUS_INVALID_PARAMETER: return CO_RC(INVALID_PARAMETER);
	}

	if (status != STATUS_SUCCESS) {
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

static co_rc_t scsi_file_detect_size(HANDLE handle, unsigned long long *out_size);

int scsi_file_open(co_scsi_dev_t *dp) {
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING unipath;
	ACCESS_MASK DesiredAccess;
	ULONG OpenOptions;
	FILE_STANDARD_INFORMATION fsi;
	co_rc_t rc;

#if DEBUG_OPEN
	printk(dp->mp, "scsi_file_open: pathname: %s\n", dp->conf->pathname);
#endif

	rc = co_winnt_utf8_to_unicode(dp->conf->pathname, &unipath);
	if (!CO_OK(rc)) return 1;

	DesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE;

	OpenOptions = FILE_SYNCHRONOUS_IO_NONALERT;

//	OpenOptions |= FILE_NO_INTERMEDIATE_BUFFERING;
//	OpenOptions |= FILE_RANDOM_ACCESS;
//	OpenOptions |= FILE_WRITE_THROUGH;
#if 0
	switch(dp->conf->type) {
	case SCSI_PTYPE_TAPE:
		OpenOptions |= FILE_SEQUENTIAL_ONLY;
		break;
	case SCSI_PTYPE_DISK:
	case SCSI_PTYPE_CDDVD:
		OpenOptions |= FILE_RANDOM_ACCESS;
		break;
	default:
		break;
	}
#endif

	/* Kernel handle needed for IoQueueWorkItem */
	InitializeObjectAttributes(&ObjectAttributes, &unipath,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwOpenFile((HANDLE *)&dp->os_handle, DesiredAccess, &ObjectAttributes, &IoStatusBlock, 0, OpenOptions);

	co_winnt_free_unicode(&unipath);

	if (status != STATUS_SUCCESS) {
		co_debug_lvl(filesystem, 5, "error %x ZwOpenFile('%s')", (int)status, dp->conf->pathname);
		return 1;
	}

#if DEBUG_OPEN
	printk(dp->mp, "scsi_file_open: os_handle: %p\n", dp->os_handle);
#endif

	status = ZwQueryInformationFile(dp->os_handle,
					&IoStatusBlock,
					&fsi,
					sizeof(fsi),
					FileStandardInformation);

	if (status == STATUS_SUCCESS) {
		dp->size = fsi.EndOfFile.QuadPart;
		co_debug("reported size: %llu KBs", (dp->size >> 10));
		rc = CO_RC(OK);
	} else {
		rc = scsi_file_detect_size(dp->os_handle, &dp->size);
		if (CO_OK(rc)) co_debug("detected size: %llu KBs", (dp->size >> 10));
	}

#if DEBUG_OPEN
	printk(dp->mp, "scsi_file_open: detected size: %lld\n", dp->size);
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

static co_rc_t transfer_file_block(co_monitor_t *cmon,
				  void *host_data, void *linuxvm,
				  unsigned long size,
				  co_monitor_transfer_dir_t dir)
{
	IO_STATUS_BLOCK isb;
	NTSTATUS status;
	co_rc_t rc = CO_RC_OK;
	LARGE_INTEGER *offset;
	scsi_worker_t *wp;

	wp = (scsi_worker_t *)host_data;
	offset = (LARGE_INTEGER *) &wp->msg;
#if DEBUG_XFER
	printk(wp->mp, "scsi_file_xfer: os_handle: %p\n", wp->dp->os_handle);
#endif

	if (CO_MONITOR_TRANSFER_FROM_HOST == dir) {
		status = ZwReadFile((HANDLE)wp->dp->os_handle,
				    NULL,
				    NULL,
				    NULL,
				    &isb,
				    linuxvm,
				    size,
				    offset,
				    NULL);
	}
	else {
		status = ZwWriteFile((HANDLE)wp->dp->os_handle,
				     NULL,
				     NULL,
				     NULL,
				     &isb,
				     linuxvm,
				     size,
				     offset,
				     NULL);
	}

#if DEBUG_XFER
	printk(wp->mp, "scsi_file_xfer: status: %x\n", status);
#endif

	if (status != STATUS_SUCCESS) {
		co_debug_error("block io failed: %p %lx (reason: %x)",
				linuxvm, size, (int)status);
		rc = status_convert(status);
	}

	offset->QuadPart += size;

	return rc;
}

co_rc_t co_monitor_host_linuxvm_transfer( co_monitor_t *cmon, void *host_data, co_monitor_transfer_func_t host_func, vm_ptr_t vaddr, unsigned long size, co_monitor_transfer_dir_t dir);

int scsi_file_rw(scsi_worker_t *wp, unsigned long long block, unsigned long num, int dir) {
	unsigned long size, len;
	co_monitor_t *mp = wp->mp;
	co_scsi_request_t *srp = &wp->req;
	co_scsi_dev_t *dp = wp->dp;
	LARGE_INTEGER *offset;
	co_rc_t rc;

	/* Calc offset */
	offset = (LARGE_INTEGER *) &wp->msg;
	offset->QuadPart = block * dp->block_size;
	size = dp->block_size * num;
	len = (size < srp->buflen ? size : srp->buflen);

#if DEBUG_XFER
	printk(wp->mp, "scsi_file_read: block: %lld\n",block);
	printk(wp->mp, "scsi_file_read: size: %ld, len: %ld\n",size,len);
#endif

	rc = co_monitor_host_linuxvm_transfer(mp, wp, transfer_file_block,
		srp->buffer, len, (dir ? CO_MONITOR_TRANSFER_FROM_LINUX : CO_MONITOR_TRANSFER_FROM_HOST));

#if DEBUG_XFER
	printk(wp->mp, "scsi_file_read: returning: %d\n", (CO_OK(rc) == 0));
#endif
	return (CO_OK(rc) == 0);
}

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

extern void scsi_worker(scsi_worker_t *);

#define Q_SIZE 256
typedef struct {
	PIO_WORKITEM pIoWorkItem;
	scsi_worker_t *wp;
} worker_info_t;
static worker_info_t wq[Q_SIZE];
static int index = 0;

static VOID DDKAPI MyRoutine(PDEVICE_OBJECT DeviceObject, PVOID Context) {
	worker_info_t *info = Context;

	scsi_worker(info->wp);
	IoFreeWorkItem(info->pIoWorkItem);
}

extern PDEVICE_OBJECT coLinux_DeviceObject;

void scsi_queue_worker(scsi_worker_t *wp, void (*func)(scsi_worker_t *)) {
	worker_info_t *info;

	info = &wq[index++];
	if (index >= Q_SIZE) index = 0;
	info->pIoWorkItem = IoAllocateWorkItem(coLinux_DeviceObject);
	info->wp = wp;

	IoQueueWorkItem(info->pIoWorkItem, MyRoutine, DelayedWorkQueue, info);
}
