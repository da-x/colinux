/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include "ddk.h"

#include <ddk/ntdddisk.h>

#include <colinux/os/alloc.h>
#include <colinux/kernel/transfer.h>
#include <colinux/kernel/fileblock.h>
#include <colinux/kernel/monitor.h>

#include "fileio.h"

static co_rc_t co_os_file_block_read(co_monitor_t *linuxvm, co_block_dev_t *dev, 
			      co_monitor_file_block_dev_t *fdev, co_block_request_t *request)
{
	return co_os_file_block_read_write(linuxvm, (HANDLE)(fdev->sysdep),
					   request->offset, request->address,
					   request->size, PTRUE);
}


static co_rc_t co_os_file_block_write(co_monitor_t *linuxvm, co_block_dev_t *dev, 
			       co_monitor_file_block_dev_t *fdev, co_block_request_t *request)
{
	return co_os_file_block_read_write(linuxvm, (HANDLE)(fdev->sysdep),
					   request->offset, request->address,
					   request->size, PFALSE);
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

static co_rc_t co_os_file_block_detect_size_binary_search(HANDLE handle, unsigned long long *out_size)
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
	unsigned long test_buffer_size_max = 0x4000;
	unsigned long test_buffer_size = 0x100;
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
		co_debug_error("size is zero\n");
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

static co_rc_t co_os_file_block_detect_size_harddisk(HANDLE handle, unsigned long long *out_size)
{
	GET_LENGTH_INFORMATION length;
	IO_STATUS_BLOCK block;
	NTSTATUS status;

	length.Length.QuadPart = 0;

	status = ZwDeviceIoControlFile(handle, NULL, NULL, NULL, &block,
				       IOCTL_DISK_GET_LENGTH_INFO,
				       NULL, 0, &length, sizeof(length));

	if (status == STATUS_SUCCESS) {
		co_debug("IOCTL_DISK_GET_LENGTH_INFO returned success\n");
		*out_size = length.Length.QuadPart;
		return CO_RC(OK);
	}

	return CO_RC(ERROR);
}

static co_rc_t co_os_file_block_detect_size(HANDLE handle, unsigned long long *out_size)
{
	co_rc_t rc;

	rc = co_os_file_block_detect_size_harddisk(handle, out_size);
	if (CO_OK(rc))
		return rc;

	/*
	 * Fall back to binary search.
	 */ 
	rc = co_os_file_block_detect_size_binary_search(handle, out_size);
	if (CO_OK(rc))
		return rc;

	*out_size = 0;
	return CO_RC(ERROR);
}

co_rc_t co_os_file_block_get_size(co_monitor_file_block_dev_t *fdev, unsigned long long *size)
{
	NTSTATUS status;
	HANDLE FileHandle;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_STANDARD_INFORMATION fsi;
	co_rc_t rc;
	bool_t opened = PFALSE;

	co_debug("device %s\n", fdev->pathname);

	if (fdev->sysdep == NULL) {
		rc = co_os_file_open(fdev->pathname, &FileHandle, FILE_READ_DATA);
		if (!CO_OK(rc))
			return rc;

		opened = TRUE;
	}
	else {
		FileHandle = (HANDLE)(fdev->sysdep);
	}

	status = ZwQueryInformationFile(FileHandle,
					&IoStatusBlock,
					&fsi,
					sizeof(fsi),
					FileStandardInformation);

	if (status == STATUS_SUCCESS) {
		*size = fsi.EndOfFile.QuadPart;
		co_debug("reported size: %llu KBs\n", (*size >> 10));
		rc = CO_RC(OK);
	}
	else {
		rc = co_os_file_block_detect_size(FileHandle, size);
		if (CO_OK(rc)) {
			co_debug("detected size: %llu KBs\n", (*size >> 10));
		}
	}
	
	if (opened)
		co_os_file_close(FileHandle);

	return rc;
}

co_rc_t co_os_file_block_open(co_monitor_t *linuxvm, co_monitor_file_block_dev_t *fdev)
{
	HANDLE FileHandle;
	co_rc_t rc;

	rc = co_os_file_open(fdev->pathname, &FileHandle, FILE_READ_DATA | FILE_WRITE_DATA);
	if (!CO_OK(rc))
		return rc;

	fdev->sysdep = (struct co_os_file_block_sysdep *)(FileHandle);
	return CO_RC(OK);
}

co_rc_t co_os_file_block_close(co_monitor_file_block_dev_t *fdev)
{
	HANDLE FileHandle;

	FileHandle = (HANDLE)(fdev->sysdep);

	co_os_file_close(FileHandle);

	fdev->sysdep = NULL;

	return CO_RC(OK);
}

co_monitor_file_block_operations_t co_os_file_block_default_operations = {
	.open = co_os_file_block_open,
	.close = co_os_file_block_close,
	.read = co_os_file_block_read,
	.write = co_os_file_block_write,
	.get_size = co_os_file_block_get_size,
};
