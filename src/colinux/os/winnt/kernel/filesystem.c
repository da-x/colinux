#include <colinux/os/alloc.h>
#include <colinux/common/libc.h>
#include <colinux/kernel/filesystem.h>
#include <colinux/os/kernel/filesystem.h>
#include <colinux/os/kernel/time.h>

#include "ddk.h"
#include <ddk/ntifs.h>
#include "time.h"
#include "fileio.h"

co_rc_t co_os_fs_inode_to_path(co_filesystem_t *fs, co_inode_t *dir, co_pathname_t *out_name)
{
	co_inode_t *dir_scan = dir;
	char *terminal = &((*out_name)[sizeof(co_pathname_t)-1]);
	char *end = terminal;
	char *beginning = &(*out_name)[0];
	char *start_adding = beginning;

	co_snprintf(start_adding, terminal - start_adding, "%s", &fs->desc->pathname[0]);
	start_adding += co_strlen(start_adding);

	*terminal = '\0';
	while (dir_scan  &&  dir_scan->name) {
		char *scan_name = dir_scan->name;
		int size = co_strlen(scan_name);
		terminal -= size;
		if (terminal < start_adding) {
			/* path too long */
			return CO_RC(ERROR);
		}
		co_memcpy(terminal, scan_name, size);
		terminal--;
		if (terminal < start_adding) {
			/* path too long */
			return CO_RC(ERROR);
		}
		*terminal = '\\';
		dir_scan = dir_scan->parent;
	}

	co_memmove(start_adding, terminal, end - terminal + 1);

	return CO_RC(OK);
}

static co_rc_t status_convert(NTSTATUS status)
{
	switch (status) {
	case STATUS_NO_SUCH_FILE: return CO_RC(NOT_FOUND);
	case STATUS_ACCESS_DENIED: return CO_RC(ACCESS_DENIED);
	case STATUS_INVALID_PARAMETER: return CO_RC(INVALID_PARAMETER);
	}

	if (status != STATUS_SUCCESS)
		return CO_RC(ERROR);

	return CO_RC(OK);
}

static int add_last_component(co_pathname_t *dirname)
{
	int len;

	len = co_strlen(*dirname);
	if (len > 0  &&  (*dirname)[len] != '\\'  && (len + 2) < sizeof(*dirname)) {
		(*dirname)[len] = '\\';
		(*dirname)[len + 1] = '\0';
		len++;
	}
	
	return len;
}

static co_rc_t dir_inode_to_path(co_filesystem_t *fs, co_inode_t *dir, 
				 co_pathname_t *out_name, char *name)
{
	co_rc_t rc;
	int len;

	rc = co_os_fs_inode_to_path(fs, dir, out_name);
	if (!CO_OK(rc))
		return rc;

	len = add_last_component(out_name);
	if (name) {
		co_snprintf(&(*out_name)[len], sizeof(*out_name) - len, "%s", name);
	}

	return CO_RC(OK);
}

static co_rc_t getattr_from_dir(co_filesystem_t *fs, co_inode_t *dir, struct fuse_attr *attr)
{
	FILE_BASIC_INFORMATION fbi;
	FILE_STANDARD_INFORMATION fsi;
	OBJECT_ATTRIBUTES attributes;
	UNICODE_STRING dirname_unicode;
	IO_STATUS_BLOCK io_status;
	ANSI_STRING ansi_string;
	NTSTATUS status;
	HANDLE handle;
	co_pathname_t dirname;
	int len;
	co_rc_t rc;

	rc = co_os_fs_inode_to_path(fs, dir, &dirname);
	if (!CO_OK(rc))
		return rc;

	len = co_strlen(&dirname[0]);

	co_debug_lvl(filesystem, 10, "%s of '%s' (%x)\n",  "getattr", &dirname[0], dir);

	ansi_string.Length = len;
	ansi_string.MaximumLength = ansi_string.Length;
	ansi_string.Buffer = &dirname[0];

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
		rc = status_convert(status);
		goto error;
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
	ZwClose(handle);

error:
	RtlFreeUnicodeString(&dirname_unicode);
	return rc;
}

co_rc_t co_os_fs_getattr(co_filesystem_t *fs, co_inode_t *dir,
			 char *name, struct fuse_attr *attr)
{
	UNICODE_STRING dirname_unicode;
	UNICODE_STRING filename_unicode;
	ANSI_STRING dirname_ansi;
	ANSI_STRING filename_ansi;
	co_pathname_t dirname;
	OBJECT_ATTRIBUTES attributes;
	NTSTATUS status;
	HANDLE handle;
	struct {
		FILE_FULL_DIRECTORY_INFORMATION entry;
		WCHAR name[0x80];
	} entry_buffer;
	IO_STATUS_BLOCK io_status;
	co_rc_t rc;
	int len;

	if (name == NULL  &&  dir->name == NULL)
		return getattr_from_dir(fs, dir, attr);

	if (name == NULL) {
		name = dir->name;
		dir = dir->parent;
	}

	if (name == NULL)
		return CO_RC(ERROR);

	rc = co_os_fs_inode_to_path(fs, dir, &dirname);
	if (!CO_OK(rc))
		return rc;

	add_last_component(&dirname);

	len = strlen(&dirname[0]);

	dirname_ansi.Length = len;
	dirname_ansi.MaximumLength = dirname_ansi.Length;
	dirname_ansi.Buffer = &dirname[0];

	status = RtlAnsiStringToUnicodeString(&dirname_unicode, &dirname_ansi, TRUE);
	if (!NT_SUCCESS(status))
		return status_convert(status);

	filename_ansi.Length = strlen(name);
	filename_ansi.MaximumLength = filename_ansi.Length;
	filename_ansi.Buffer = name;

	status = RtlAnsiStringToUnicodeString(&filename_unicode, &filename_ansi, TRUE);
	if (!NT_SUCCESS(status)) {
		rc = status_convert(status);
		goto error_1;
	}

	co_debug_lvl(filesystem, 10, "getattr %s of '%s'", &dirname[0], name);

	InitializeObjectAttributes(&attributes, &dirname_unicode,
				   OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwCreateFile(&handle, FILE_LIST_DIRECTORY,
			      &attributes, &io_status, NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE /* TODO: add | FILE_SHARE_DELETE */, 
			      FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE, 
			      NULL, 0);
	
	if (!NT_SUCCESS(status)) {
		rc = status_convert(status);
		goto error_2;
	}

	status = ZwQueryDirectoryFile(handle, NULL, NULL, 0, &io_status, 
				      &entry_buffer, sizeof(entry_buffer), 
				      FileFullDirectoryInformation, PTRUE, &filename_unicode, 
				      PTRUE);
	if (!NT_SUCCESS(status)) {
		rc = status_convert(status);
		goto error_3;
	}

	attr->uid = 0;
	attr->gid = 0;
	attr->rdev = 0;
	attr->_dummy = 0;

	attr->atime = windows_time_to_unix_time(entry_buffer.entry.LastAccessTime);
	attr->mtime = windows_time_to_unix_time(entry_buffer.entry.LastWriteTime);
	attr->ctime = windows_time_to_unix_time(entry_buffer.entry.ChangeTime);

	attr->mode = FUSE_S_IRWXU | FUSE_S_IRGRP | FUSE_S_IROTH;

	if (entry_buffer.entry.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		attr->mode |= FUSE_S_IFDIR;
	else
		attr->mode |= FUSE_S_IFREG;
	
	attr->nlink = 1;
	attr->size = entry_buffer.entry.EndOfFile.QuadPart;
	attr->blocks = (entry_buffer.entry.EndOfFile.QuadPart + ((1<<10)-1)) >> 10;

	rc = CO_RC(OK);

error_3:
	ZwClose(handle);
error_2:
	RtlFreeUnicodeString(&filename_unicode);
error_1:
	RtlFreeUnicodeString(&dirname_unicode);
	return rc;
}

co_rc_t co_os_fs_getdir(co_filesystem_t *fs, co_inode_t *dir, co_filesystem_dir_names_t *names)
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
	co_pathname_t dirname;
	co_filesystem_name_t *new_name;
	co_rc_t rc;
	int len;

	co_list_init(&names->list);

	rc = co_os_fs_inode_to_path(fs, dir, &dirname);
	if (!CO_OK(rc))
		return rc;

	len = add_last_component(&dirname);
	
	ansi_string.Length = len;
	ansi_string.MaximumLength = ansi_string.Length;
	ansi_string.Buffer = &dirname[0];

	co_debug_lvl(filesystem, 10, "listing of '%s'\n", dirname);

	status = RtlAnsiStringToUnicodeString(&dirname_unicode, &ansi_string, TRUE);
	if (!NT_SUCCESS(status))
		return CO_RC(ERROR);

	InitializeObjectAttributes(&attributes, &dirname_unicode,
				   OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwCreateFile(&handle, FILE_LIST_DIRECTORY,
			      &attributes, &io_status, NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE /* TODO: add | FILE_SHARE_DELETE */, 
			      FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE, 
			      NULL, 0);
	
	if (!NT_SUCCESS(status)) {
		rc = CO_RC(ERROR);
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
				rc = CO_RC(ERROR);
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


co_rc_t co_os_fs_inode_read_write(co_monitor_t *linuxvm, co_filesystem_t *filesystem, co_inode_t *inode, 
				  unsigned long long offset, unsigned long size,
				  vm_ptr_t src_buffer, bool_t read)
{
	co_pathname_t filename;
	co_rc_t rc;
	NTSTATUS status;
	HANDLE handle;

	rc = co_os_fs_inode_to_path(filesystem, inode, &filename);
	if (!CO_OK(rc))
		return rc;

	status = co_os_open_file(filename, &handle, read ? FILE_READ_DATA : FILE_WRITE_DATA);

	if (status != STATUS_SUCCESS)
		return status_convert(status);

	rc = co_os_file_block_read_write(linuxvm,
					 handle,
					 offset,
					 src_buffer,
					 size,
					 read);

	co_os_close_file(handle);

	return rc;
}

co_rc_t co_os_fs_inode_mknod(co_filesystem_t *filesystem, co_inode_t *inode, unsigned long mode, 
			     unsigned long rdev, char *name, int *ino, struct fuse_attr *attr)
{
	co_pathname_t dirname;
	NTSTATUS status;
	HANDLE handle;
	co_rc_t rc;

	rc = dir_inode_to_path(filesystem, inode, &dirname, name);
	if (!CO_OK(rc))
		return rc;

	status = co_os_create_file(dirname, &handle, FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES,
				   FILE_ATTRIBUTE_NORMAL, FILE_CREATE, 0);

	if (status != STATUS_SUCCESS) {
		rc = status_convert(status);
	} else {
		ZwClose(handle);
	}

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

co_rc_t co_os_fs_inode_set_attr(co_filesystem_t *filesystem, co_inode_t *inode,
				unsigned long valid, struct fuse_attr *attr)
{
	IO_STATUS_BLOCK io_status;
	NTSTATUS status;
	co_pathname_t filename;
	co_rc_t rc;

	rc = co_os_fs_inode_to_path(filesystem, inode, &filename);
	if (!CO_OK(rc))
		return rc;

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

		status = co_os_change_file_information(filename, &io_status, &fbi,
						       sizeof(fbi), FileBasicInformation,
						       change_file_info_func,
						       attr);

		rc = status_convert(status);
	}

	if (valid & FATTR_SIZE) {
		FILE_END_OF_FILE_INFORMATION feofi;
		
		feofi.EndOfFile.QuadPart = attr->size;

		status = co_os_set_file_information(filename, &io_status, &feofi,
						    sizeof(feofi), FileEndOfFileInformation);

		rc = status_convert(status);
	} 

	return rc;
} 

co_rc_t co_os_fs_inode_mkdir(co_filesystem_t *filesystem, co_inode_t *inode, unsigned long mode, 
			     char *name)
{
	co_pathname_t dirname;
	NTSTATUS status;
	HANDLE handle;
	co_rc_t rc;

	rc = dir_inode_to_path(filesystem, inode, &dirname, name);
	if (!CO_OK(rc))
		return rc;

	status = co_os_create_file(dirname, &handle, FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES,
				   FILE_ATTRIBUTE_DIRECTORY, FILE_CREATE, FILE_DIRECTORY_FILE);

	if (status != STATUS_SUCCESS) {
		rc = status_convert(status);
	} else {
		ZwClose(handle);
	}	

	return rc;
}

co_rc_t co_os_fs_inode_unlink(co_filesystem_t *filesystem, co_inode_t *inode, char *name)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING unipath;
	ANSI_STRING ansi;
	co_pathname_t filename;
	co_rc_t rc;
	NTSTATUS status;

	rc = dir_inode_to_path(filesystem, inode, &filename, name);
	if (!CO_OK(rc))
		return rc;

	ansi.Buffer = filename;
	ansi.Length = strlen(filename);
	ansi.MaximumLength = ansi.Length + 1;
	
	status = RtlAnsiStringToUnicodeString(&unipath, &ansi, TRUE);
	if (!NT_SUCCESS(status))
		return CO_RC(ERROR);

	co_debug_lvl(filesystem, 10, "unlink: '%s'\n", filename);

	InitializeObjectAttributes(&ObjectAttributes, 
				   &unipath,
				   OBJ_CASE_INSENSITIVE,
				   NULL,
				   NULL);

	status = ZwDeleteFile(&ObjectAttributes);
	if (status != STATUS_SUCCESS)
		co_debug_lvl(filesystem, 10, "ZwDeleteFile() returned error: %x\n", status);

	RtlFreeUnicodeString(&unipath);
	return status;
}

co_rc_t co_os_fs_inode_rmdir(co_filesystem_t *filesystem, co_inode_t *inode, char *name)
{
	return co_os_fs_inode_unlink(filesystem, inode, name);
}

co_rc_t co_os_fs_inode_rename(co_filesystem_t *filesystem, 
			      co_inode_t *dir, co_inode_t *new_dir,
			      char *oldname, char *newname)
{
	NTSTATUS status;
	IO_STATUS_BLOCK io_status;
	FILE_RENAME_INFORMATION *rename_info;
	UNICODE_STRING filename_unicode;
	HANDLE handle;
	ANSI_STRING ansi_string;
	co_pathname_t filename;
	co_pathname_t dest_filename;
	int block_size;
	co_rc_t rc;

	rc = dir_inode_to_path(filesystem, dir, &filename, oldname);
	if (!CO_OK(rc))
		return rc;

	rc = dir_inode_to_path(filesystem, new_dir, &dest_filename, newname);
	if (!CO_OK(rc))
		return rc;

	block_size = (strlen(dest_filename) + 1)*sizeof(WCHAR) + sizeof(FILE_RENAME_INFORMATION);
	rename_info = (FILE_RENAME_INFORMATION *)co_os_malloc(block_size);
	if (!rename_info)
		return CO_RC(OUT_OF_MEMORY);

	status = co_os_open_file(filename, &handle, FILE_READ_DATA | FILE_WRITE_DATA);
	if (status != STATUS_SUCCESS) {
		rc = status_convert(status);
		goto error;
	}

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
		rc = CO_RC(ERROR);
		goto error;
	}
	
	co_debug_lvl(filesystem, 10, "rename of '%s' to '%s'\n", filename, dest_filename);

	status = ZwSetInformationFile(handle, &io_status,
				      rename_info, block_size, 
				      FileRenameInformation);
	rc = status_convert(status);
error:
	co_os_free(rename_info);

	ZwClose(handle);

	return rc;
}

