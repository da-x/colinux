/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 
 
#include <windows.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

#include <colinux/common.h>

#include "driver.h"
 
co_rc_t 
coui_install_driver( 
	IN SC_HANDLE  SchSCManager, 
	IN LPCTSTR    DriverName, 
	IN LPCTSTR    ServiceExe 
	) 
{ 
	SC_HANDLE  schService; 
	DWORD      err; 
 
	schService = CreateService (SchSCManager,         
				    DriverName,           
				    DriverName,          
				    SERVICE_ALL_ACCESS,  
				    SERVICE_KERNEL_DRIVER, 
				    SERVICE_AUTO_START,    
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
 
	if (DeleteService (schService))
		rc = CO_RC(OK);
	else
		rc = CO_RC(ERROR_REMOVING_DRIVER);
 
	CloseServiceHandle (schService); 
 
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
			coui_debug ("failure: StartService, ERROR_SERVICE_ALREADY_RUNNING\n"); 
		else 
			coui_debug ("failure: StartService (0x%02x)\n", err);
#endif
	} 
 
	CloseServiceHandle (schService); 

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
	else
		rc = CO_RC(ERROR_STOPPING_DRIVER);

	CloseServiceHandle(schService); 
 
	return rc; 
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
 
	schSCManager = OpenSCManager (NULL,                 // machine (NULL == local) 
				      NULL,                 // database (NULL == default) 
				      SC_MANAGER_ALL_ACCESS // access required 
		); 
 
	coui_stop_driver(schSCManager, name);  
	coui_remove_driver(schSCManager, name);
	
	CloseServiceHandle(schSCManager); 
    
	return CO_RC(OK);
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
	if (!CO_OK(rc))
		return rc;

	rc = coui_start_driver(schSCManager, name); 
	if (!CO_OK(rc)) {
		coui_remove_driver(schSCManager, name); 
		return rc;
	}

	rc = coui_check_device(name); 
	if (!CO_OK(rc)) {
		coui_stop_driver(schSCManager, name);  
		coui_remove_driver(schSCManager, name); 
		return rc;
	}

	CloseServiceHandle(schSCManager);    

	return CO_RC(OK);
} 

co_rc_t coui_unload_driver(void)
{
	return coui_unload_driver_by_name(CO_DRIVER_NAME);
}
 
co_rc_t coui_load_driver(void)
{
	return coui_load_driver_by_name(CO_DRIVER_NAME, COLINUX_DRIVER_FILE);
}

