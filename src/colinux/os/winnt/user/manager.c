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

static int open_handles = 0;

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
  
	if (rc == FALSE)
		return CO_RC(ERROR);

	return CO_RC(OK);
}

co_rc_t 
coui_install_driver( 
	IN SC_HANDLE  SchSCManager, 
	IN LPCTSTR    DriverName, 
	IN LPCTSTR    ServiceExe 
	) 
{ 
	SC_HANDLE  schService; 
 
	schService = CreateService (SchSCManager,         
				    DriverName,           
				    DriverName,          
				    SERVICE_ALL_ACCESS,
				    SERVICE_KERNEL_DRIVER, 
				    SERVICE_DEMAND_START,    
				    SERVICE_ERROR_NORMAL, 
				    ServiceExe,           
				    NULL,
				    NULL,
				    NULL,
				    NULL,
				    NULL); 
 
	if (schService == NULL) 
		return CO_RC(ERROR_INSTALLING_DRIVER); 
 
	/* Possible error: ERROR_SERVICE_EXISTS */
 
	CloseServiceHandle(schService);

	return CO_RC(OK); 
} 
 
 
co_rc_t 
coui_remove_driver( 
	IN SC_HANDLE  SchSCManager, 
	IN LPCTSTR    DriverName 
	) 
{ 
	SC_HANDLE  schService; 
	co_rc_t   rc;

	schService = OpenService (SchSCManager, 
				  DriverName, 
				  SERVICE_ALL_ACCESS); 
 
	if (schService == NULL)
		return CO_RC(ERROR); 
 
	if (DeleteService(schService))
		rc = CO_RC(OK);
	else
		rc = CO_RC(ERROR_REMOVING_DRIVER);
 
	CloseServiceHandle(schService); 
 
	return rc; 
} 
 

co_rc_t coui_start_driver( 
	IN SC_HANDLE  SchSCManager, 
	IN LPCTSTR    DriverName 
	) 
{ 
	SC_HANDLE  schService; 
	co_rc_t   ret;
	DWORD      err; 
    
	schService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS); 
 
	if (schService == NULL) { 
		return CO_RC(ERROR_ACCESSING_DRIVER); 
	} 
 
	if (StartService(schService, 0, NULL))
		ret = CO_RC(OK);
	else
		ret = CO_RC(ERROR_STARTING_DRIVER);

	if (!ret) {
		err = GetLastError(); 

#if (0) 
		if (err == ERROR_SERVICE_ALREADY_RUNNING) 
			coui_debug("failure: StartService, ERROR_SERVICE_ALREADY_RUNNING\n"); 
		else 
			coui_debug("failure: StartService (0x%02x)\n", err);
#endif
	} 
 
	CloseServiceHandle(schService); 

	return ret; 
} 
 
co_rc_t coui_stop_driver( 
	IN SC_HANDLE  SchSCManager, 
	IN LPCTSTR    DriverName 
	) 
{ 
	SC_HANDLE       schService; 
	SERVICE_STATUS  serviceStatus; 
	co_rc_t        rc;

	schService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS); 
 
	if (schService == NULL)
		return CO_RC(ERROR_ACCESSING_DRIVER); 

	if (ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus))
		rc = CO_RC(OK);
	else {
		rc = CO_RC(ERROR_STOPPING_DRIVER);
	}

	CloseServiceHandle(schService); 
 
	return rc; 
} 
 
co_rc_t coui_check_driver(IN LPCTSTR DriverName, bool_t *installed) 
{ 
	SC_HANDLE schService; 
	SC_HANDLE schSCManager; 

	*installed = PFALSE;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL)
		return CO_RC(ERROR_ACCESSING_DRIVER); 

	schService = OpenService(schSCManager, DriverName, SERVICE_ALL_ACCESS); 
	if (schService != NULL) {
		CloseServiceHandle(schService); 

		*installed = PTRUE;
	}
	
	CloseServiceHandle(schSCManager); 
	return CO_RC(OK);
} 

co_rc_t coui_check_device(IN LPCTSTR DriverName) 
{ 
	char     completeDeviceName[64] = ""; 
	LPCTSTR  dosDeviceName = DriverName; 
	HANDLE   hDevice; 
	co_rc_t ret;
 
	strcat(completeDeviceName, "\\\\.\\"); 
	strcat(completeDeviceName, dosDeviceName); 
 
	hDevice = CreateFile(completeDeviceName, 
			      GENERIC_READ | GENERIC_WRITE, 
			      0, 
			      NULL, 
			      OPEN_EXISTING, 
			      FILE_ATTRIBUTE_NORMAL, 
			      NULL 
		); 
 
	if (hDevice == ((HANDLE)-1)) { 
		ret = CO_RC(ERROR_ACCESSING_DRIVER); 
	} else { 
		CloseHandle(hDevice); 
		ret = CO_RC(OK); 
	} 
 
	return ret; 
} 

co_rc_t coui_unload_driver_by_name(char *name) 
{ 
	SC_HANDLE   schSCManager; 
	co_rc_t rc;

	schSCManager = OpenSCManager (NULL,                 // machine (NULL == local) 
				      NULL,                 // database (NULL == default) 
				      SC_MANAGER_ALL_ACCESS // access required 
		); 
 
	co_debug("driver: stopping driver service:\n");
	rc = coui_stop_driver(schSCManager, name);  

	co_debug("driver: removing driver service:\n");
	rc = coui_remove_driver(schSCManager, name);
	
	CloseServiceHandle(schSCManager); 

	/*
	 * Apparently this givens the service manager an opportunity to
	 * remove the service before we reinstall it.
	 */
	Sleep(100);
			
	return rc;
} 

co_rc_t coui_load_driver_by_name(char *name, char *path) 
{ 
	SC_HANDLE   schSCManager; 
	char fullpath[0x100] = {0,};
	co_rc_t rc;
 
	schSCManager = OpenSCManager (NULL,                 // machine (NULL == local) 
				      NULL,                 // database (NULL == default) 
				      SC_MANAGER_ALL_ACCESS /* access required */ ); 
    
	GetFullPathName(path, sizeof(fullpath), fullpath, NULL);
	
	rc = coui_install_driver(schSCManager, name, fullpath);
	if (!CO_OK(rc)) {
		CloseServiceHandle(schSCManager);    
		return rc;
	}

	rc = coui_start_driver(schSCManager, name); 
	if (!CO_OK(rc)) {
		coui_remove_driver(schSCManager, name); 
		CloseServiceHandle(schSCManager);    
		return rc;
	}

	rc = coui_check_device(name); 
	if (!CO_OK(rc)) {
		coui_stop_driver(schSCManager, name);  
		coui_remove_driver(schSCManager, name); 
		CloseServiceHandle(schSCManager);    
		return rc;
	}

	CloseServiceHandle(schSCManager);    

	return CO_RC(OK);
} 

co_rc_t co_os_manager_is_installed(bool_t *installed)
{
	return coui_check_driver(CO_DRIVER_NAME, installed);
}

co_rc_t co_os_manager_remove(void)
{
	return coui_unload_driver_by_name(CO_DRIVER_NAME);
}

co_rc_t co_os_manager_install(void)
{
	return coui_load_driver_by_name(CO_DRIVER_NAME, COLINUX_DRIVER_FILE);
}
