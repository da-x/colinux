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

NTSTATUS co_os_create_file(char *pathname, PHANDLE FileHandle, unsigned long open_flags,
			   unsigned long file_attribute, unsigned long create_disposition,
			   unsigned long options)
{    
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING unipath;
	ANSI_STRING ansi;
    
	ansi.Buffer = pathname;
	ansi.Length = strlen(pathname);
	ansi.MaximumLength = ansi.Length + 1;

	status = RtlAnsiStringToUnicodeString(&unipath, &ansi, TRUE);
	if (!NT_SUCCESS(status))
		return CO_RC(ERROR);

	InitializeObjectAttributes(&ObjectAttributes, 
				   &unipath,
				   OBJ_CASE_INSENSITIVE,
				   NULL,
				   NULL);

	status = ZwCreateFile(FileHandle, 
			      open_flags | SYNCHRONIZE,
			      &ObjectAttributes,
			      &IoStatusBlock,
			      NULL,
			      file_attribute,
			      0,
			      create_disposition,
			      options | FILE_SYNCHRONOUS_IO_NONALERT,
			      NULL,
			      0);

	if (status != STATUS_SUCCESS)
		co_debug("ZwOpenFile() returned error: %x\n", status);

	RtlFreeUnicodeString(&unipath);

	return status;
}

NTSTATUS co_os_open_file(char *pathname, PHANDLE FileHandle, unsigned long open_flags)
{   
	return co_os_create_file(pathname, FileHandle, open_flags, 0, FILE_OPEN, 0);
}

NTSTATUS co_os_close_file(PHANDLE FileHandle)
{    
	NTSTATUS status = 0;

	status = ZwClose(FileHandle);

	return status;
}

typedef struct {
	LARGE_INTEGER offset;
	HANDLE file_handle;
} co_os_transfer_file_block_data_t;

static co_rc_t transfer_file_block(co_monitor_t *cmon, 
				  void *host_data, void *linuxvm, 
				  unsigned long size, 
				  co_monitor_transfer_dir_t dir)
{
	IO_STATUS_BLOCK isb;
	NTSTATUS status;
	co_os_transfer_file_block_data_t *data;
	co_rc_t rc = CO_RC_OK;
	HANDLE FileHandle;

	data = (co_os_transfer_file_block_data_t *)host_data;

	FileHandle = data->file_handle;

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

co_rc_t co_os_file_block_read_write(co_monitor_t *linuxvm,
				    HANDLE file_handle,
				    unsigned long long offset,
				    vm_ptr_t address,
				    unsigned long size,
				    bool_t read)
{
	co_rc_t rc;
	co_os_transfer_file_block_data_t data;
	
	data.offset.QuadPart = offset;
	data.file_handle = file_handle;

	rc = co_monitor_host_linuxvm_transfer(linuxvm, 
					      &data, 
					      transfer_file_block,
					      address,
					      size,
					      (read ? CO_MONITOR_TRANSFER_FROM_HOST : 
					       CO_MONITOR_TRANSFER_FROM_LINUX));

	return rc;
}

NTSTATUS co_os_change_file_information(char *filename,
				       IO_STATUS_BLOCK *io_status,
				       VOID *buffer,
				       ULONG len,
				       FILE_INFORMATION_CLASS info_class,
				       co_os_change_file_info_func_t func,
				       void *data)
{
	NTSTATUS status;
	HANDLE handle;

	status = co_os_open_file(filename, &handle, FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES);
	if (status != STATUS_SUCCESS) {
		return status;
	}

	status = ZwQueryInformationFile(handle, io_status, buffer, len, info_class);
	if (status == STATUS_SUCCESS) {
		if (func) {
			func(data, buffer, len);
		}
		status = ZwSetInformationFile(handle, io_status, buffer, len, info_class);
	}

	ZwClose(handle);

	return status;
}


NTSTATUS co_os_set_file_information(char *filename,
				    IO_STATUS_BLOCK *io_status,
				    VOID *buffer,
				    ULONG len,
				    FILE_INFORMATION_CLASS info_class)
{
	NTSTATUS status;
	HANDLE handle;

	status = co_os_open_file(filename, &handle, FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES);
	if (status != STATUS_SUCCESS) {
		return status;
	}

	status = ZwSetInformationFile(handle, io_status, buffer, len, info_class);
	status = ZwSetInformationFile(handle, io_status, buffer, len, info_class);
		
	co_debug_system("%x\n", status);

	ZwClose(handle);

	return status;
}

