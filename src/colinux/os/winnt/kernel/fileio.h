/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_WINNT_KERNEL_FILEIO_H__
#define __CO_WINNT_KERNEL_FILEIO_H__

#include <colinux/kernel/monitor.h>

#include "ddk.h"

extern NTSTATUS co_os_open_file(char *pathname, PHANDLE FileHandle,
				unsigned long open_flags);

extern NTSTATUS co_os_create_file(char *pathname, PHANDLE FileHandle, unsigned long open_flags,
				  unsigned long file_attribute, unsigned long create_disposition,
				  unsigned long options);

extern NTSTATUS co_os_close_file(PHANDLE FileHandle);

extern co_rc_t co_os_file_block_read_write(co_monitor_t *linuxvm,
					   HANDLE file_handle,
					   unsigned long long offset,
					   vm_ptr_t address,
					   unsigned long size,
					   bool_t read);

extern NTSTATUS co_os_set_file_information(char *filename,
					   IO_STATUS_BLOCK *io_status,
					   VOID *buffer,
					   ULONG len,
					   FILE_INFORMATION_CLASS info_class);

typedef void (*co_os_change_file_info_func_t)(void *data, VOID *buffer, ULONG len);

extern NTSTATUS co_os_change_file_information(char *filename,
					      IO_STATUS_BLOCK *io_status,
					      VOID *buffer,
					      ULONG len,
					      FILE_INFORMATION_CLASS info_class,
					      co_os_change_file_info_func_t func,
					      void *data);

#endif
