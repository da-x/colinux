/*
 * This source code is a part of coLinux source package.
 *
 * Henry Nestler, 2008 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>
#include <colinux/os/user/config.h>
#include <colinux/os/user/misc.h>

#define SOFTWARE_COLINUX_MONITOR_KEY "SOFTWARE\\coLinux\\monitor"

co_rc_t co_config_user_string_read(int		monitor_index,
				   const char*  device_name,
				   int 		device_index,
				   const char*	value_name,
				   char*	value,
				   int 		size)
{
	char  key_name[256];
	HKEY  key;
	DWORD name_type;
	LONG  status;

	co_snprintf(key_name,
	            sizeof(key_name),
		    SOFTWARE_COLINUX_MONITOR_KEY "\\%d\\%s%d",
		    monitor_index,
		    device_name,
		    device_index);

        status = RegOpenKeyEx(
			HKEY_CURRENT_USER,
			key_name,
			0,
			KEY_READ,
			&key);

        if (status != ERROR_SUCCESS) {
		co_debug("error (%ld) opening registry key for read: HCU\\%s (%ld)\n",
			 status, key_name, status);
		return CO_RC(ERROR);
	}

	status = RegQueryValueEx(
			key,
			value_name,
			NULL,
			&name_type,
			(PBYTE)value,
			(DWORD*)&size);

	RegCloseKey(key);

	if (status != ERROR_SUCCESS || name_type != REG_SZ) {
		co_terminal_print("error (%ld) reading registry key: HCU\\%s\\%s\n",
				  status, key_name, value_name);
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

co_rc_t co_config_user_string_write(int 	monitor_index,
		                    const char* device_name,
		                    int 	device_index,
		                    const char* value_name,
		                    const char* value)
{
	char key_name[256];
	HKEY key;
	LONG status;

	co_snprintf(key_name,
	            sizeof(key_name),
		    SOFTWARE_COLINUX_MONITOR_KEY "\\%d\\%s%d",
		    monitor_index,
		    device_name,
		    device_index);

	status = RegCreateKeyEx(
			HKEY_CURRENT_USER,
			key_name,
        		0,
			NULL,
			REG_OPTION_NON_VOLATILE,
        		KEY_WRITE,
			NULL,
			&key,
			NULL);

        if (status != ERROR_SUCCESS) {
		co_terminal_print("error (%ld) creating registry key: HCU\\%s\n", status, key_name);
    		return CO_RC(ERROR);
	}

	status = RegSetValueEx(
			key,
    			value_name,
			0,
        		REG_SZ,
			(BYTE*)value,
        		(DWORD)strlen(value)+1);

	RegCloseKey(key);

	if (status != ERROR_SUCCESS) {
		co_terminal_print("error (%ld) writing registry key: HCU\\%s\\%s\n",
				  status, key_name, value_name);
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}
