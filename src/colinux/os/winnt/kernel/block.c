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

#include <colinux/os/alloc.h>
#include <colinux/kernel/transfer.h>
#include <colinux/kernel/fileblock.h>

NTSTATUS co_os_open_file(char *pathname, PHANDLE FileHandle)
{    
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING unipath;
	ANSI_STRING ansi;
    
	ansi.Buffer = pathname;
	ansi.Length = strlen(pathname);
	ansi.MaximumLength = ansi.Length + 1;

	RtlAnsiStringToUnicodeString(&unipath, &ansi, TRUE);

	InitializeObjectAttributes(&ObjectAttributes, 
				   &unipath,
				   OBJ_CASE_INSENSITIVE,
				   NULL,
				   NULL);

	status = ZwCreateFile(FileHandle, 
			      FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
			      &ObjectAttributes,
			      &IoStatusBlock,
			      NULL,
			      0,
			      0,
			      FILE_OPEN,
			      FILE_SYNCHRONOUS_IO_NONALERT,
			      NULL,
			      0);

	if (status != STATUS_SUCCESS)
		co_debug("ZwOpenFile() returned error: %x\n", status);

	RtlFreeUnicodeString(&unipath);

	return status;
}

NTSTATUS co_os_close_file(PHANDLE FileHandle)
{    
	NTSTATUS status = 0;

	status = ZwClose(FileHandle);

	return status;
}

typedef struct {
	LARGE_INTEGER offset;
	co_monitor_file_block_dev_t *fdev;
} co_os_transfer_file_block_data_t;

co_rc_t co_os_transfer_file_block(co_monitor_t *cmon, 
				  void *host_data, void *linuxvm, unsigned long size, 
				  co_monitor_transfer_dir_t dir)
{
	IO_STATUS_BLOCK isb;
	NTSTATUS status;
	co_os_transfer_file_block_data_t *data;
	co_rc_t rc = CO_RC_OK;
	HANDLE FileHandle;

	data = (co_os_transfer_file_block_data_t *)host_data;

	FileHandle = (HANDLE)(data->fdev->sysdep);

	if (CO_MONITOR_TRANSFER_FROM_HOST == dir) {
		status = ZwReadFile(FileHandle,
				    NULL,
				    NULL,
				    NULL, 
				    &isb,
				    linuxvm,
				    size,
				    &data->offset,
				    NULL);
	}
	else {
		status = ZwWriteFile(FileHandle,
				     NULL,
				     NULL,
				     NULL, 
				     &isb,
				     linuxvm,
				     size,
				     &data->offset,
				     NULL);
	}

	if (status != STATUS_SUCCESS) {
		co_debug("block io failed: %x %x (reason: %x)\n", linuxvm, size,
			   status);
		rc = CO_RC(ERROR);
	}

	data->offset.QuadPart += size;

	return rc;
}


co_rc_t co_os_file_block_read(co_monitor_t *linuxvm,
			     co_block_dev_t *dev, 
			     co_monitor_file_block_dev_t *fdev,
			     co_block_request_t *request)
{
	co_rc_t rc;
	co_os_transfer_file_block_data_t data;
	
	data.offset.QuadPart = request->offset;
	data.fdev = fdev;

	rc = co_monitor_host_linuxvm_transfer(linuxvm, 
					   &data, 
					   co_os_transfer_file_block,
					   request->address,
					   (unsigned long)request->size,
					   CO_MONITOR_TRANSFER_FROM_HOST);

	return rc;
}


co_rc_t co_os_file_block_write(co_monitor_t *linuxvm,
			       co_block_dev_t *dev, 
			       co_monitor_file_block_dev_t *fdev,
			       co_block_request_t *request)
{
	co_rc_t rc;
	co_os_transfer_file_block_data_t data;
	
	data.offset.QuadPart = request->offset;
	data.fdev = fdev;
	
	rc = co_monitor_host_linuxvm_transfer(linuxvm,
					   &data, 
					   co_os_transfer_file_block,
					   request->address,
					   (unsigned long)request->size,
					   CO_MONITOR_TRANSFER_FROM_LINUX);
	return rc;
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

static co_rc_t co_os_file_block_detect_size(HANDLE handle, unsigned long long *out_size)
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
	unsigned long test_buffer_size_max = 0x1000;
	unsigned long test_buffer_size = 0x100;
	char *test_buffer;

	test_buffer = (char *)co_os_malloc(test_buffer_size_max);
	if (!test_buffer)
		return CO_RC(ERROR);

	build_size.QuadPart = 0;

	while (test_buffer_size <= test_buffer_size_max) {
		if (probe_area(handle, build_size, test_buffer, test_buffer_size))
			break;

		test_buffer_size <<= 1;
	}

	if (test_buffer_size >= test_buffer_size_max) {
		co_debug("%s: size is zero\n", __FUNCTION__);
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

co_rc_t co_os_file_block_get_size(co_monitor_file_block_dev_t *fdev, unsigned long long *size)
{
	NTSTATUS status;
	HANDLE FileHandle;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_STANDARD_INFORMATION fsi;
	co_rc_t rc;
	bool_t opened = PFALSE;

	co_debug("%s: device %s\n", __FUNCTION__, fdev->pathname);

	if (fdev->sysdep == NULL) {
		status = co_os_open_file(fdev->pathname, &FileHandle);
		if (status != STATUS_SUCCESS)
			return CO_RC(ERROR);

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
		co_debug("%s: reported size: %llu bytes\n", __FUNCTION__, *size);
		rc = CO_RC(OK);
	}
	else {
		rc = co_os_file_block_detect_size(FileHandle, size);
		if (CO_OK(rc)) {
			co_debug("%s: detected size: %llu bytes\n", __FUNCTION__, *size);
		}
	}
	
	if (opened)
		co_os_close_file(FileHandle);

	return rc;
}

co_rc_t co_os_file_block_open(co_monitor_t *linuxvm, co_monitor_file_block_dev_t *fdev)
{
	HANDLE FileHandle;
	NTSTATUS status;

	status = co_os_open_file(fdev->pathname, &FileHandle);
	if (status != STATUS_SUCCESS)
		return CO_RC(ERROR);

	fdev->sysdep = (struct co_os_file_block_sysdep *)(FileHandle);
	return CO_RC(OK);
}

co_rc_t co_os_file_block_close(co_monitor_file_block_dev_t *fdev)
{
	HANDLE FileHandle;

	FileHandle = (HANDLE)(fdev->sysdep);

	co_os_close_file(FileHandle);

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
