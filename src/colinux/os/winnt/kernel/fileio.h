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

co_rc_t co_os_file_get_size(HANDLE handle, unsigned long long *out_size);

co_rc_t co_winnt_utf8_to_unicode(const char *src, UNICODE_STRING *unicode_str);
void co_winnt_free_unicode(UNICODE_STRING *unicode_str);
co_rc_t co_status_convert(NTSTATUS status);

extern co_rc_t co_os_file_open(char *pathname, PHANDLE FileHandle,
			       unsigned long open_flags);

extern co_rc_t co_os_file_create(char *pathname, PHANDLE FileHandle, unsigned long open_flags,
				 unsigned long file_attribute, unsigned long create_disposition,
				 unsigned long options);

extern co_rc_t co_os_file_block_read_write(co_monitor_t *monitor,
					   HANDLE file_handle,
					   unsigned long long offset,
					   vm_ptr_t address,
					   unsigned long size,
					   bool_t read);

extern co_rc_t co_os_file_close(PHANDLE FileHandle);

#endif
