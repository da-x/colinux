/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 * Orignally based on code from OpenVPN,
 * Copyright (C) 2002-2003 James Yonan <jim@yonan.net>
 */

#include <stdio.h>
#include <windows.h>

#include <ddk/ntapi.h>
#include <ddk/winddk.h>
#include <ddk/ntddk.h>

#include <colinux/common/common.h>
#include <colinux/os/user/misc.h>

#include "../../kernel/tap-win32/constants.h"
#include "../../kernel/tap-win32/common.h"

bool_t
is_tap_win32_dev(const char *guid)
{
	HKEY netcard_key;
	LONG status;
	DWORD len;
	int i = 0;

	status = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		ADAPTER_KEY,
		0,
		KEY_READ,
		&netcard_key);

	if (status != ERROR_SUCCESS) {
		printf("Error opening registry key: %s\n", ADAPTER_KEY);
		return PFALSE;
	}

	while (PTRUE)
	{
		char enum_name[256];
		char unit_string[256];
		HKEY unit_key;
		char component_id_string[] = "ComponentId";
		char component_id[256];
		char net_cfg_instance_id_string[] = "NetCfgInstanceId";
		char net_cfg_instance_id[256];
		DWORD data_type;

		len = sizeof (enum_name);
		status = RegEnumKeyEx(
			netcard_key,
			i,
			enum_name,
			&len,
			NULL,
			NULL,
			NULL,
			NULL);

		if (status == ERROR_NO_MORE_ITEMS)
			break;
		else if (status != ERROR_SUCCESS) {
			printf("Error enumerating registry subkeys of key: %s\n",
				ADAPTER_KEY);
			return PFALSE;
		}
	
		snprintf (unit_string, sizeof(unit_string), "%s\\%s",
			  ADAPTER_KEY, enum_name);

		status = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			unit_string,
			0,
			KEY_READ,
			&unit_key);

		if (status != ERROR_SUCCESS) {
			printf("Error opening registry key: %s\n", unit_string); 
			return PFALSE;
		}
		else
		{
			len = sizeof (component_id);
			status = RegQueryValueEx(
				unit_key,
				component_id_string,
				NULL,
				&data_type,
				component_id,
				&len);

			if (!(status != ERROR_SUCCESS || data_type != REG_SZ)) {
				len = sizeof (net_cfg_instance_id);
				status = RegQueryValueEx(
					unit_key,
					net_cfg_instance_id_string,
					NULL,
					&data_type,
					net_cfg_instance_id,
					&len);

				if (status == ERROR_SUCCESS && data_type == REG_SZ)
				{
					if (!strcmp (component_id, "tapdev")
					    && !strcmp (net_cfg_instance_id, guid))
					{
						RegCloseKey (unit_key);
						RegCloseKey (netcard_key);
						return PTRUE;
					}
				}
			}
			RegCloseKey (unit_key);
		}
		++i;
	}

	RegCloseKey (netcard_key);
	return PFALSE;
}

co_rc_t get_device_guid(
	char *name,
	int name_size,
	char *actual_name,
	int actual_name_size)
{
	LONG status;
	HKEY control_net_key;
	DWORD len;
	int i = 0;
	int stop = 0;

	status = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		NETWORK_CONNECTIONS_KEY,
		0,
		KEY_READ,
		&control_net_key);

	if (status != ERROR_SUCCESS) {
		printf("Error opening registry key: %s", NETWORK_CONNECTIONS_KEY);
		return CO_RC(ERROR);
	}

	while (!stop)
	{
		char enum_name[256];
		char connection_string[256];
		HKEY connection_key;
		char name_data[256];
		DWORD name_type;
		const char name_string[] = "Name";

		len = sizeof (enum_name);
		status = RegEnumKeyEx(
			control_net_key,
			i,
			enum_name,
			&len,
			NULL,
			NULL,
			NULL,
			NULL);

		if (status == ERROR_NO_MORE_ITEMS)
			break;
		else if (status != ERROR_SUCCESS) {
			printf("Error enumerating registry subkeys of key: %s",
			       NETWORK_CONNECTIONS_KEY);
			return CO_RC(ERROR);
		}

		snprintf(connection_string, 
			 sizeof(connection_string),
			 "%s\\%s\\Connection",
			 NETWORK_CONNECTIONS_KEY, enum_name);

		status = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			connection_string,
			0,
			KEY_READ,
			&connection_key);
		
		if (status == ERROR_SUCCESS) {
			len = sizeof (name_data);
			status = RegQueryValueEx(
				connection_key,
				name_string,
				NULL,
				&name_type,
				name_data,
				&len);

			if (status != ERROR_SUCCESS || name_type != REG_SZ) {
				printf("Error opening registry key: %s\\%s\\%s",
				       NETWORK_CONNECTIONS_KEY, connection_string, name_string);
			        return CO_RC(ERROR);
			}
			else {
				if (is_tap_win32_dev(enum_name)) {
					co_terminal_print("conet-daemon: found TAP device named \"%s\"\n", name_data);

					snprintf(name, name_size, "%s", enum_name);
					if (actual_name) {
						if (strcmp(actual_name, "") != 0) {
							if (strcmp(name_data, actual_name) != 0) {
								RegCloseKey (connection_key);
								++i;
								continue;
							}
						}
						else {
							snprintf(actual_name, actual_name_size, "%s", name_data);
						}
					}
					stop = 1;
				}
			}

			RegCloseKey (connection_key);
		}
		++i;
	}

	RegCloseKey (control_net_key);

	if (stop == 0)
		return CO_RC(ERROR);

	return CO_RC(OK); 
}

co_rc_t open_tap_win32(HANDLE *phandle, char *prefered_name)
{
	char device_path[256];
	char device_guid[0x100];
	co_rc_t rc;
	HANDLE handle;
	BOOL bret;
	char name_buffer[0x100] = {0, };
	struct {
		unsigned long major;
		unsigned long minor;
		unsigned long debug;		
	} version;
	int version_len;

	if (prefered_name != NULL)
		co_snprintf(name_buffer, sizeof(name_buffer), "%s", prefered_name);

	rc = get_device_guid(device_guid, sizeof(device_guid), name_buffer, sizeof(name_buffer));
	if (!CO_OK(rc))
		return CO_RC(NOT_FOUND);

	co_debug("conet-daemon: opening TAP: \"%s\"\n", name_buffer);

	/*
	 * Open Windows TAP-Win32 adapter
	 */
	snprintf (device_path, sizeof(device_path), "%s%s%s",
		  USERMODEDEVICEDIR,
		  device_guid,
		  TAPSUFFIX);

	handle = CreateFile (
		device_path,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,
		0
		);

	if (handle == 0)
		return CO_RC(ERROR);

	bret = DeviceIoControl(handle, TAP_IOCTL_GET_VERSION,
			       &version, sizeof (version),
			       &version, sizeof (version), &version_len, NULL);

	if (bret == FALSE) {
		CloseHandle(handle);
		co_debug("conet-daemon: error getting driver version\n");
		return CO_RC(ERROR);
	}

	co_debug("conet-daemon: driver version %d.%d\n", version.major, version.minor);
	
	*phandle = handle;

	return CO_RC(OK);	
}


BOOL tap_win32_set_status(HANDLE handle, BOOL status)
{
	unsigned long len = 0;

	return DeviceIoControl(handle, TAP_IOCTL_SET_MEDIA_STATUS,
				&status, sizeof (status),
				&status, sizeof (status), &len, NULL);
}
