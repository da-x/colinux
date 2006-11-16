/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */ 

#include <windows.h>
#include <winioctl.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/manager.h>
#include <colinux/os/current/kernel/driver.h>
#include <colinux/os/current/os.h>
#include <colinux/os/current/ioctl.h>

#include "misc.h"

struct co_manager_handle {
	HANDLE handle;
};

co_manager_handle_t co_os_manager_open(void)
{
	co_manager_handle_t handle;

	handle = co_os_malloc(sizeof(*handle));
	if (!handle)
		return NULL;

	handle->handle = CreateFile(CO_DRIVER_USER_PATH, 
				    GENERIC_READ | GENERIC_WRITE, 0, NULL, 
				    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle->handle == INVALID_HANDLE_VALUE) {
		co_terminal_print_last_error("colinux: manager open");
		co_os_free(handle);
		return NULL;
	}

	return handle;
}

void co_os_manager_close(co_manager_handle_t handle)
{
	CloseHandle(handle->handle);
	co_os_free(handle);
}

co_rc_t co_os_manager_ioctl(
	co_manager_handle_t kernel_device,
	unsigned long code,
	void *input_buffer,
	unsigned long input_buffer_size,
	void *output_buffer,
	unsigned long output_buffer_size,
	unsigned long *output_returned)
{
	DWORD   BytesReturned = 0;
	BOOL    rc;

	if (output_returned == NULL)
		output_returned = &BytesReturned;

	code = CO_WINNT_IOCTL(code);

	rc = DeviceIoControl(kernel_device->handle, code,
			     input_buffer, input_buffer_size, 
			     output_buffer, output_buffer_size, 
			     output_returned, NULL);

	if (rc == FALSE) {
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}
 
co_rc_t co_winnt_check_driver(IN LPCTSTR DriverName, bool_t *installed) 
{ 
	SC_HANDLE schService; 
	SC_HANDLE schSCManager; 

	*installed = PFALSE;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL) {
		DWORD last_error = GetLastError();
		if (last_error == ERROR_ACCESS_DENIED)
			return CO_RC(ACCESS_DENIED);

		return CO_RC(ERROR_ACCESSING_DRIVER); 
	}

	schService = OpenService(schSCManager, DriverName, SERVICE_ALL_ACCESS); 
	if (schService != NULL) {
		CloseServiceHandle(schService); 
		*installed = PTRUE;
	}

	CloseServiceHandle(schSCManager);
	return CO_RC(OK);
}

co_rc_t co_os_manager_is_installed(bool_t *installed)
{
	return co_winnt_check_driver(CO_DRIVER_NAME, installed);
}
