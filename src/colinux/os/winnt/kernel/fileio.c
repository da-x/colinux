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
#include <ddk/ntifs.h>
#include <ddk/ntdddisk.h>

#include <colinux/os/alloc.h>
#include <colinux/common/libc.h>
#include <colinux/kernel/transfer.h>
#include <colinux/kernel/fileblock.h>
#include <colinux/kernel/monitor.h>
#include <colinux/kernel/filesystem.h>
#include <colinux/os/kernel/time.h>

#include "time.h"
#include "fileio.h"

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
		co_debug_system("STATUS %x\n", status);
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

co_rc_t co_os_file_create(char *pathname, PHANDLE FileHandle, unsigned long open_flags,
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
		return status_convert(status);

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

	return status_convert(status);
}

co_rc_t co_os_file_open(char *pathname, PHANDLE FileHandle, unsigned long open_flags)
{   
	return co_os_file_create(pathname, FileHandle, open_flags, 0, FILE_OPEN, 0);
}

co_rc_t co_os_file_close(PHANDLE FileHandle)
{    
	NTSTATUS status = 0;

	status = ZwClose(FileHandle);

	return status_convert(status);
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
		rc = status_convert(status);
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

typedef void (*co_os_change_file_info_func_t)(void *data, VOID *buffer, ULONG len);

co_rc_t co_os_change_file_information(char *filename,
				      IO_STATUS_BLOCK *io_status,
				      VOID *buffer,
				      ULONG len,
				      FILE_INFORMATION_CLASS info_class,
				      co_os_change_file_info_func_t func,
				      void *data)
{
	NTSTATUS status;
	HANDLE handle;
	co_rc_t rc;

	rc = co_os_file_open(filename, &handle, FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES);
	if (!CO_OK(rc)) {
		rc = co_os_file_open(filename, &handle, FILE_WRITE_ATTRIBUTES);
		if (!CO_OK(rc))
			return rc;
	}

	status = ZwQueryInformationFile(handle, io_status, buffer, len, info_class);
	if (status == STATUS_SUCCESS) {
		if (func) {
			func(data, buffer, len);
		}
		status = ZwSetInformationFile(handle, io_status, buffer, len, info_class);
	}

	co_os_file_close(handle);

	return status_convert(status);
}

co_rc_t co_os_set_file_information(char *filename,
				   IO_STATUS_BLOCK *io_status,
				   VOID *buffer,
				   ULONG len,
				   FILE_INFORMATION_CLASS info_class)
{
	HANDLE handle;
	NTSTATUS status;
	co_rc_t rc;

	rc = co_os_file_open(filename, &handle, FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES);
	if (!CO_OK(rc))
		return rc;

	status = ZwSetInformationFile(handle, io_status, buffer, len, info_class);

	co_os_file_close(handle);

	return status_convert(status);
}

co_rc_t co_os_file_read_write(co_monitor_t *linuxvm, char *filename, 
			      unsigned long long offset, unsigned long size,
			      vm_ptr_t src_buffer, bool_t read)
{
	co_rc_t rc;
	HANDLE handle;

	rc = co_os_file_open(filename, &handle, read ? FILE_READ_DATA : FILE_WRITE_DATA);
	if (!CO_OK(rc))
		return rc;

	rc = co_os_file_block_read_write(linuxvm,
					 handle,
					 offset,
					 src_buffer,
					 size,
					 read);

	co_os_file_close(handle);

	return rc;
}

static void change_file_info_func(void *data, VOID *buffer, ULONG len)
{
	struct fuse_attr *attr = (struct fuse_attr *)data;
	FILE_BASIC_INFORMATION *fbi = (FILE_BASIC_INFORMATION *)buffer;

	fbi->LastAccessTime = unix_time_to_windows_time(attr->mtime);
	fbi->LastWriteTime = unix_time_to_windows_time(attr->atime);
	KeQuerySystemTime(&fbi->ChangeTime);
}

co_rc_t co_os_file_set_attr(char *filename, unsigned long valid, struct fuse_attr *attr)
{
	IO_STATUS_BLOCK io_status;
	co_rc_t rc = CO_RC(OK);

	/* FIXME: make return codes not to overwrite each other */
	if (valid & FATTR_MODE) {
		rc = CO_RC(OK); /* TODO */
	}

	if (valid & FATTR_UID) {
		rc = CO_RC(OK); /* TODO */
	}

	if (valid & FATTR_GID) {
		rc = CO_RC(OK); /* TODO */
	}

	if (valid & FATTR_UTIME) {
		FILE_BASIC_INFORMATION fbi;

		rc = co_os_change_file_information(filename, &io_status, &fbi,
						   sizeof(fbi), FileBasicInformation,
						   change_file_info_func,
						   attr);
	}

	if (valid & FATTR_SIZE) {
		FILE_END_OF_FILE_INFORMATION feofi;
		
		feofi.EndOfFile.QuadPart = attr->size;

		rc = co_os_set_file_information(filename, &io_status, &feofi,
						sizeof(feofi), FileEndOfFileInformation);
	} 

	return rc;
} 

co_rc_t co_os_file_get_attr(char *filename, struct fuse_attr *attr)
{
	FILE_BASIC_INFORMATION fbi;
	FILE_STANDARD_INFORMATION fsi;
	OBJECT_ATTRIBUTES attributes;
	UNICODE_STRING dirname_unicode;
	IO_STATUS_BLOCK io_status;
	ANSI_STRING ansi_string;
	NTSTATUS status;
	HANDLE handle;
	int len;
	co_rc_t rc = CO_RC(OK);

	len = co_strlen(&filename[0]);

	co_debug_lvl(filesystem, 10, "%s of '%s'\n",  "getattr", &filename[0]);

	ansi_string.Length = len;
	ansi_string.MaximumLength = ansi_string.Length;
	ansi_string.Buffer = &filename[0];

	status = RtlAnsiStringToUnicodeString(&dirname_unicode, &ansi_string, TRUE);
	if (!NT_SUCCESS(status))
		return status_convert(status);

	InitializeObjectAttributes(&attributes, &dirname_unicode,
				   OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwCreateFile(&handle, FILE_READ_ATTRIBUTES,
			      &attributes, &io_status, NULL, 0, FILE_SHARE_READ | 
			      FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
			      FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, 
			      NULL, 0);

	if (!NT_SUCCESS(status)) {
		status = ZwCreateFile(&handle, FILE_READ_ATTRIBUTES,
				      &attributes, &io_status, NULL, 0, FILE_SHARE_READ | 
				      FILE_SHARE_DELETE, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, 
				      NULL, 0);
		if (!NT_SUCCESS(status)) {
			status = ZwCreateFile(&handle, FILE_READ_ATTRIBUTES,
					      &attributes, &io_status, NULL, 0, 0, 
					      FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT,
					      NULL, 0);
			if (!NT_SUCCESS(status)) {
				rc = status_convert(status);
				goto error;
			}

			rc = status_convert(status);
			goto error;
		}
	}

	status = ZwQueryInformationFile(handle, &io_status, &fbi,
					sizeof(fbi), FileBasicInformation);
	if (!NT_SUCCESS(status)) {
		rc = status_convert(status);
		goto error_2;
	}

	attr->uid = 0;
	attr->gid = 0;
	attr->rdev = 0;
	attr->_dummy = 0;
	attr->blocks = 0;

	attr->atime = windows_time_to_unix_time(fbi.LastAccessTime);
	attr->mtime = windows_time_to_unix_time(fbi.LastWriteTime);
	attr->ctime = windows_time_to_unix_time(fbi.ChangeTime);

	attr->mode = FUSE_S_IRGRP | FUSE_S_IXGRP | FUSE_S_IRWXU | FUSE_S_IROTH | FUSE_S_IWOTH;

	if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		attr->mode |= FUSE_S_IFDIR;
	else
		attr->mode |= FUSE_S_IFREG;

	status = ZwQueryInformationFile(handle, &io_status, &fsi,
					sizeof(fsi), FileStandardInformation);
	if (!NT_SUCCESS(status)) {
		rc = CO_RC(ERROR);
		goto error_2;
	}
	
	attr->nlink = 1;
	attr->size = fsi.EndOfFile.QuadPart;
	attr->blocks = (fsi.EndOfFile.QuadPart + ((1<<10)-1)) >> 10;

error_2:
	co_os_file_close(handle);

error:
	RtlFreeUnicodeString(&dirname_unicode);

	return rc;
}

static void remove_read_only_func(void *data, VOID *buffer, ULONG len)
{
	FILE_BASIC_INFORMATION *fbi = (FILE_BASIC_INFORMATION *)buffer;

	fbi->FileAttributes &= ~FILE_ATTRIBUTE_READONLY;
}

co_rc_t co_os_file_unlink(char *filename)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING unipath;
	ANSI_STRING ansi;
	NTSTATUS status;
	bool_t tried_read_only_removal = PFALSE;

	ansi.Buffer = filename;
	ansi.Length = strlen(filename);
	ansi.MaximumLength = ansi.Length + 1;
	
	status = RtlAnsiStringToUnicodeString(&unipath, &ansi, TRUE);
	if (!NT_SUCCESS(status))
		return status_convert(status);

	InitializeObjectAttributes(&ObjectAttributes, 
				   &unipath,
				   OBJ_CASE_INSENSITIVE,
				   NULL,
				   NULL);

retry:

	status = ZwDeleteFile(&ObjectAttributes);
	if (status != STATUS_SUCCESS) {
		if (!tried_read_only_removal) {
			FILE_BASIC_INFORMATION fbi;
			IO_STATUS_BLOCK io_status;
			co_rc_t rc;
			
			rc = co_os_change_file_information(filename, &io_status, &fbi,
							   sizeof(fbi), FileBasicInformation,
							   remove_read_only_func,
							   NULL);

			tried_read_only_removal = PTRUE;
			if (CO_RC(OK))
				goto retry;
		}

		co_debug_lvl(filesystem, 10, "ZwDeleteFile() returned error: %x\n", status);
	}

	RtlFreeUnicodeString(&unipath);

	return status_convert(status);
}

co_rc_t co_os_file_rmdir(char *filename)
{
	return co_os_file_unlink(filename);
}

co_rc_t co_os_file_mkdir(char *dirname)
{
	NTSTATUS status;
	HANDLE handle;
	co_rc_t rc = CO_RC(OK);

	status = co_os_file_create(dirname, &handle, FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES,
				   FILE_ATTRIBUTE_DIRECTORY, FILE_CREATE, FILE_DIRECTORY_FILE);

	if (status != STATUS_SUCCESS) {
		rc = status_convert(status);
	} else {
		co_os_file_close(handle);
	}	

	return rc;
}

co_rc_t co_os_file_rename(char *filename, char *dest_filename)
{
	NTSTATUS status;
	IO_STATUS_BLOCK io_status;
	FILE_RENAME_INFORMATION *rename_info;
	UNICODE_STRING filename_unicode;
	HANDLE handle;
	ANSI_STRING ansi_string;
	int block_size;
	co_rc_t rc;

	block_size = (strlen(dest_filename) + 1)*sizeof(WCHAR) + sizeof(FILE_RENAME_INFORMATION);
	rename_info = (FILE_RENAME_INFORMATION *)co_os_malloc(block_size);
	if (!rename_info)
		return CO_RC(OUT_OF_MEMORY);

	rc = co_os_file_open(filename, &handle, FILE_READ_DATA | FILE_WRITE_DATA);
	if (!CO_OK(rc))
		goto error;

	rename_info->ReplaceIfExists = FALSE;
	rename_info->RootDirectory = NULL;
	rename_info->FileNameLength = strlen(dest_filename)*sizeof(WCHAR);

	filename_unicode.Buffer = rename_info->FileName;
	filename_unicode.MaximumLength = (strlen(dest_filename)+1)*sizeof(WCHAR);
	filename_unicode.Length = 0;

	ansi_string.Length = co_strlen(dest_filename);
	ansi_string.MaximumLength = ansi_string.Length;
	ansi_string.Buffer = dest_filename;
			
	status = RtlAnsiStringToUnicodeString(&filename_unicode, &ansi_string, FALSE);
	if (!NT_SUCCESS(status)) {
		rc = status_convert(status);
		goto error;
	}
	
	co_debug_lvl(filesystem, 10, "rename of '%s' to '%s'\n", filename, dest_filename);

	status = ZwSetInformationFile(handle, &io_status, rename_info, block_size,
				      FileRenameInformation);
	rc = status_convert(status);

error:
	co_os_free(rename_info);
	co_os_file_close(handle);
	return rc;
}

co_rc_t co_os_file_mknod(char *filename)
{
	co_rc_t rc;
	HANDLE handle;

	rc = co_os_file_create(filename, &handle, FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES,
			       FILE_ATTRIBUTE_NORMAL, FILE_CREATE, 0);

	if (CO_OK(rc)) {
		co_os_file_close(handle);
	}

	return rc;
}

co_rc_t co_os_file_getdir(char *dirname, co_filesystem_dir_names_t *names)
{
	UNICODE_STRING dirname_unicode;
	UNICODE_STRING filename_unicode;
	OBJECT_ATTRIBUTES attributes;
	NTSTATUS status;
	HANDLE handle;
	FILE_DIRECTORY_INFORMATION *dir_entries_buffer, *entry;
	unsigned long dir_entries_buffer_size = 0x1000;
	IO_STATUS_BLOCK io_status;
	BOOLEAN first_iteration = TRUE;
	ANSI_STRING ansi_string;
	co_filesystem_name_t *new_name;
	co_rc_t rc;

	co_list_init(&names->list);

	ansi_string.Length = strlen(dirname);
	ansi_string.MaximumLength = ansi_string.Length;
	ansi_string.Buffer = &dirname[0];

	co_debug_lvl(filesystem, 10, "listing of '%s'\n", dirname);

	status = RtlAnsiStringToUnicodeString(&dirname_unicode, &ansi_string, TRUE);
	if (!NT_SUCCESS(status))
		return status_convert(status);

	InitializeObjectAttributes(&attributes, &dirname_unicode,
				   OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwCreateFile(&handle, FILE_LIST_DIRECTORY,
			      &attributes, &io_status, NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE /* TODO: add | FILE_SHARE_DELETE */, 
			      FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE, 
			      NULL, 0);
	
	if (!NT_SUCCESS(status)) {
		rc = status_convert(status);
		goto error;
	}

	dir_entries_buffer = co_os_malloc(dir_entries_buffer_size);
	if (!dir_entries_buffer) {
		rc = CO_RC(OUT_OF_MEMORY);
		goto error_1;
	}

	for (;;) {
		status = ZwQueryDirectoryFile(handle, NULL, NULL, 0, &io_status, 
					      dir_entries_buffer, dir_entries_buffer_size, 
					      FileDirectoryInformation, FALSE, NULL, first_iteration);
		if (!NT_SUCCESS(status))
			break;

		entry = dir_entries_buffer;
  
		for (;;) {
			filename_unicode.Buffer = entry->FileName;
			filename_unicode.MaximumLength = entry->FileNameLength;
			filename_unicode.Length = entry->FileNameLength;

			new_name = co_os_malloc(entry->FileNameLength/sizeof(WCHAR) + sizeof(co_filesystem_name_t) + 2);
			if (!new_name) {
				rc = CO_RC(ERROR);
				goto error_2;
			}
			
			if (entry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				new_name->type = FUSE_DT_DIR;
			else
				new_name->type = FUSE_DT_REG;

			ansi_string.Length = 0;
			ansi_string.MaximumLength = entry->FileNameLength/sizeof(WCHAR) + 1;
			ansi_string.Buffer = &new_name->name[0];
			
			status = RtlUnicodeStringToAnsiString(&ansi_string, &filename_unicode, FALSE);
			if (!NT_SUCCESS(status)) {
				rc = status_convert(status);
				co_os_free(new_name);
				goto error_2;
			}

			ansi_string.Buffer[ansi_string.Length] = '\0';
	
			co_list_add_tail(&new_name->node, &names->list);
			
			if (entry->NextEntryOffset == 0)
				break;
			
			entry = (FILE_DIRECTORY_INFORMATION *)(((char *)entry) + entry->NextEntryOffset);
		} 

		first_iteration = FALSE;
	}


	rc = CO_RC(OK);

error_2:
	if (!CO_OK(rc))
		co_filesystem_getdir_free(names);

	co_os_free(dir_entries_buffer);

error_1:
	ZwClose(handle);
error:
	RtlFreeUnicodeString(&dirname_unicode);
	return rc;
}

co_rc_t co_os_file_fs_stat(co_filesystem_t *filesystem, struct fuse_statfs_out *statfs)
{
	FILE_FS_FULL_SIZE_INFORMATION fsi;
	HANDLE handle;
	NTSTATUS status;
	IO_STATUS_BLOCK io_status;
	co_pathname_t pathname;
	co_rc_t rc;
	int len;
	
	memcpy(&pathname, &filesystem->base_path, sizeof(co_pathname_t));

	len = strlen(pathname);
	do {
		rc = co_os_file_create(pathname, &handle, 
				       FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE,
				       FILE_OPEN, FILE_DIRECTORY_FILE | FILE_OPEN_FOR_FREE_SPACE_QUERY);
		
		if (CO_OK(rc)) 
			break;
		
		while (len > 0  &&  pathname[len-1] != '\\')
			len--;

		pathname[len] = '\0';
	} while (len > 0);
	
	status = ZwQueryVolumeInformationFile(handle, &io_status, &fsi,
					      sizeof(fsi), FileFsFullSizeInformation);

	if (NT_SUCCESS(status)) {
		statfs->st.block_size = fsi.SectorsPerAllocationUnit * fsi.BytesPerSector;
		statfs->st.blocks = fsi.TotalAllocationUnits.QuadPart;
		statfs->st.blocks_free = fsi.CallerAvailableAllocationUnits.QuadPart;
		statfs->st.files = 0;
		statfs->st.files_free = 0;
		statfs->st.namelen = sizeof(co_pathname_t);
	}

	rc = status_convert(status);

	co_os_file_close(handle);
	return rc;
}
