/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include "ddk.h"

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
				  void *host_data, void *colx, unsigned long size, 
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
				    colx,
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
				     colx,
				     size,
				     &data->offset,
				     NULL);
	}

	if (status != STATUS_SUCCESS) {
		co_debug("block io failed: %x %x (reason: %x)\n", colx, size,
			   status);
		rc = CO_RC(ERROR);
	}

	data->offset.QuadPart += size;

	return rc;
}


co_rc_t co_os_file_block_read(co_monitor_t *colx,
			     co_block_dev_t *dev, 
			     co_monitor_file_block_dev_t *fdev,
			     co_block_request_t *request)
{
	co_rc_t rc;
	co_os_transfer_file_block_data_t data;
	
	data.offset.QuadPart = request->offset;
	data.fdev = fdev;
	
	rc = co_monitor_host_colx_transfer(colx, 
					   &data, 
					   co_os_transfer_file_block,
					   request->address,
					   (unsigned long)request->size,
					   CO_MONITOR_TRANSFER_FROM_HOST);

	return rc;
}


co_rc_t co_os_file_block_write(co_monitor_t *colx,
			       co_block_dev_t *dev, 
			       co_monitor_file_block_dev_t *fdev,
			       co_block_request_t *request)
{
	co_rc_t rc;
	co_os_transfer_file_block_data_t data;
	
	data.offset.QuadPart = request->offset;
	data.fdev = fdev;
	
	rc = co_monitor_host_colx_transfer(colx,
					   &data, 
					   co_os_transfer_file_block,
					   request->address,
					   (unsigned long)request->size,
					   CO_MONITOR_TRANSFER_FROM_COLX);
	return rc;
}

co_rc_t co_os_file_block_get_size(co_monitor_file_block_dev_t *fdev, unsigned long long *size)
{
	NTSTATUS status;
	HANDLE FileHandle;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_STANDARD_INFORMATION fsi;
	co_rc_t rc;

	status = co_os_open_file(fdev->pathname, &FileHandle);
	if (status != STATUS_SUCCESS)
		return CO_RC(ERROR);

	status = ZwQueryInformationFile(FileHandle,
					&IoStatusBlock,
					&fsi,
					sizeof(fsi),
					FileStandardInformation);


	if (status == STATUS_SUCCESS) {
		*size = fsi.EndOfFile.QuadPart;
		rc = CO_RC(OK);
	}
	else
		rc = CO_RC(ERROR);

	co_os_close_file(FileHandle);
	return rc;
}

co_rc_t co_os_file_block_open(co_monitor_t *colx, co_monitor_file_block_dev_t *fdev)
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

	return CO_RC(OK);
}

co_monitor_file_block_operations_t co_os_file_block_default_operations = {
	.open = co_os_file_block_open,
	.close = co_os_file_block_close,
	.read = co_os_file_block_read,
	.write = co_os_file_block_write,
	.get_size = co_os_file_block_get_size,
};
