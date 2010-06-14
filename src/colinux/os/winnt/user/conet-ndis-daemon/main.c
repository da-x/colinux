/*
 * This source code is a part of coLinux source package.
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>
#include <winsock2.h>

#include <stdio.h>

#include <colinux/common/common.h>
#include <colinux/user/debug.h>
#include <colinux/user/reactor.h>
#include <colinux/user/monitor.h>
#include <colinux/user/macaddress.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/current/user/reactor.h>
#include "pcap-registry.h"

COLINUX_DEFINE_MODULE("colinux-ndis-net-daemon");

/*******************************************************************************
 * Type Declarations
 */

/* Runtime parameters */

typedef struct start_parameters {
	bool_t show_help;
	bool_t mac_specified;
	char mac_address[18];
	bool_t name_specified;
	char interface_name[0x100];
	co_id_t instance;
	int index;
	int promisc;
} start_parameters_t;

/*******************************************************************************
 * Globals
 */
co_reactor_t g_reactor = NULL;
co_user_monitor_t *g_monitor_handle = NULL;
start_parameters_t *daemon_parameters;
char netcfg_id[256];

static co_rc_t monitor_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	return CO_RC(OK);
}

/*******************************************************************************
 * Get the Connection name for an Bridge Interface (using NetCfgInstanceId).
 */
#define CONET_NAME_BUF_LEN 256 /* allows to use strcpy */
static co_rc_t conet_init(void)
{
	LONG status;
	HKEY control_net_key;
	DWORD len;
	int i;
	char device_guid[CONET_NAME_BUF_LEN];
	char name_buffer[CONET_NAME_BUF_LEN];

	status = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		NETWORK_CONNECTIONS_KEY,
		0,
		KEY_READ,
		&control_net_key);

	if (status != ERROR_SUCCESS) {
		co_terminal_print("conet-ndis-daemon: Error opening registry key: %s\n", NETWORK_CONNECTIONS_KEY);
		return CO_RC(ERROR);
	}

	name_buffer[0] = 0;
	for(i = 0; i < 1000; i++)
	{
		char enum_name[CONET_NAME_BUF_LEN];
		char connection_string[CONET_NAME_BUF_LEN];
		HKEY connection_key;
		char name_data[CONET_NAME_BUF_LEN];
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
			co_terminal_print("conet-ndis-daemon: Error enumerating registry subkeys of key: %s\n",
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
				(LPBYTE)name_data,
				&len);

			if (status != ERROR_SUCCESS || name_type != REG_SZ) {
				co_terminal_print("conet-ndis-daemon: Error opening registry key: %s\\%s\\%s\n",
				       NETWORK_CONNECTIONS_KEY, connection_string, name_string);
			        return CO_RC(ERROR);
			}
			else {
				co_debug("Ndis bridge probe on \"%s\"", name_data);

				if (daemon_parameters->name_specified == PTRUE) {
					/*
					 If an exact match exists, over-ride any found device,
					  setting exact match device to it.
					 */
					if (strcmp(name_data, daemon_parameters->interface_name) == 0) {
						strcpy(name_buffer, name_data);
						strcpy(device_guid, enum_name);
						break;
					}

					/*
					  Do an partial search, if partial search is found,
					   set this device as he found device, but continue
					   looping through devices.
					 */

					if (!*name_buffer &&
					    strstr(name_data, daemon_parameters->interface_name) != NULL) {
						strcpy(name_buffer, name_data);
						strcpy(device_guid, enum_name);
					}
				} else {
					/*
					 If no name specified and network has an address,
					  autoselect first device.
					*/
					strcpy(name_buffer, name_data);
					strcpy(device_guid, enum_name);
					break;
				}
			}

			RegCloseKey (connection_key);
		}
	}

	RegCloseKey (control_net_key);

	if (!*name_buffer)
		return CO_RC(ERROR);

	snprintf(netcfg_id, sizeof(netcfg_id), "\\Device\\%s", device_guid);
	co_terminal_print("conet-ndis-daemon: Bridge on: %s\n", name_buffer);
	if (daemon_parameters->promisc == 0)	// promiscuous mode?
		co_terminal_print("conet-ndis-daemon: Promiscuous mode disabled\n");

	return CO_RC(OK);
}

/********************************************************************************
 * parameters
 */

static void co_net_syntax()
{
	co_terminal_print("Cooperative Linux Bridged Network Daemon\n");
	co_terminal_print("Ligong Liu, 2008 (c)\n");
	co_terminal_print("\n");
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("  colinux-ndis-net-daemon -i pid -u unit [-h] [-n 'adapter name'] [-mac xx:xx:xx:xx:xx:xx] [-p promiscuous]\n");
	co_terminal_print("\n");
	co_terminal_print("    -h                      Show this help text\n");
	co_terminal_print("    -i pid                  coLinux instance ID to connect to\n");
	co_terminal_print("    -u unit                 Network device index number (0 for eth0, 1 for\n");
	co_terminal_print("                            eth1, etc.)\n");
	co_terminal_print("    -n 'adapter name'       The name of the network adapter to attach to\n");
	co_terminal_print("    -mac xx:xx:xx:xx:xx:xx  MAC address for the bridged interface\n");
	co_terminal_print("    -p mode                 Promiscuous mode can disable with mode 0, if\n");
	co_terminal_print("                            have problems. It's 1 (enabled) by default\n");
}

static co_rc_t
handle_parameters(start_parameters_t *start_parameters, int argc, char *argv[])
{
	char **param_scan = argv;
	const char *option;

	/* Default settings */
	start_parameters->index = -1;
	start_parameters->instance = -1;
	start_parameters->show_help = PFALSE;
	start_parameters->mac_specified = PFALSE;
	start_parameters->name_specified = PFALSE;
	start_parameters->promisc = 1;

	/* Parse command line */
	while (*param_scan) {
		option = "-mac";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-ndis-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			co_snprintf(start_parameters->mac_address,
				    sizeof(start_parameters->mac_address),
				    "%s", *param_scan);

			start_parameters->mac_specified = PTRUE;
			param_scan++;
			continue;
		}

		option = "-u";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-ndis-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			start_parameters->index = atoi(*param_scan);
			param_scan++;
			continue;
		}

		option = "-i";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-ndis-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			start_parameters->instance = (co_id_t) atoi(*param_scan);
			param_scan++;
			continue;
		}

		option = "-n";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-ndis-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			co_snprintf(start_parameters->interface_name,
				    sizeof(start_parameters->interface_name),
				    "%s", *param_scan);

			start_parameters->name_specified = PTRUE;
			param_scan++;
			continue;
		}

		option = "-p";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("conet-ndis-daemon: parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			start_parameters->promisc = atoi(*param_scan);
			param_scan++;
			continue;
		}

		option = "-h";
		if (strcmp(*param_scan, option) == 0) {
			start_parameters->show_help = PTRUE;
			return CO_RC(OK);
		}
		param_scan++;
	}

	if (start_parameters->index == -1) {
		co_terminal_print("conet-ndis-daemon: device index not specified\n");
		return CO_RC(ERROR);
	}

	if ((start_parameters->index < 0) ||
	    (start_parameters->index >= CO_MODULE_MAX_CONET))
	{
		co_terminal_print("conet-ndis-daemon: invalid index: %d\n", start_parameters->index);
		return CO_RC(ERROR);
	}

	if (start_parameters->instance == -1) {
		co_terminal_print("conet-ndis-daemon: coLinux instance not specificed\n");
		return CO_RC(ERROR);
	}

	if (!start_parameters->mac_specified) {
		co_terminal_print("conet-ndis-daemon: error, MAC address not specified\n");
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

static int
conet_ndis_main(int argc, char *argv[])
{
	co_rc_t rc;
	start_parameters_t start_parameters;
	co_module_t modules[] = {CO_MODULE_CONET0, };
	co_monitor_ioctl_conet_bind_adapter_t ioctl;
	int a0,a1,a2,a3,a4,a5;

	rc = handle_parameters(&start_parameters, argc, argv);
	if (!CO_OK(rc))
		return -1;

	if (start_parameters.show_help) {
		co_net_syntax();
		return 0;
	}

	daemon_parameters = &start_parameters;

	rc = conet_init();
	if (!CO_OK(rc)) {
		co_terminal_print("conet-ndis-daemon: Error initializing ndis-bridge (rc %x)\n", (int)rc);
		return -1;
	}

	rc = co_reactor_create(&g_reactor);
	if (!CO_OK(rc))
		return -1;

	modules[0] += start_parameters.index;
	rc = co_user_monitor_open(g_reactor, monitor_receive,
				  start_parameters.instance, modules,
				  sizeof(modules)/sizeof(co_module_t),
				  &g_monitor_handle);
	if (!CO_OK(rc))
		return -1;

	/* send ioctl to monitor bind conet protocol to adapter */
	ioctl.conet_proto = CO_CONET_BRIDGE;
	ioctl.conet_unit = start_parameters.index;
	ioctl.promisc_mode = start_parameters.promisc;
	sscanf(start_parameters.mac_address, "%2x:%2x:%2x:%2x:%2x:%2x",
			&a0, &a1, &a2, &a3, &a4, &a5);
	ioctl.mac_address[0] = a0;
	ioctl.mac_address[1] = a1;
	ioctl.mac_address[2] = a2;
	ioctl.mac_address[3] = a3;
	ioctl.mac_address[4] = a4;
	ioctl.mac_address[5] = a5;
	strcpy(ioctl.netcfg_id, netcfg_id);

	co_user_monitor_conet_bind_adapter(g_monitor_handle, &ioctl);

	while (1) {
		rc = co_reactor_select(g_reactor, -1);
		if (!CO_OK(rc))
			break;
	}

	co_user_monitor_close(g_monitor_handle);
	co_reactor_destroy(g_reactor);

	return 0;
}

/********************************************************************************
 * main...
 */

int
main(int argc, char *argv[])
{
	int ret;

	co_debug_start();

	ret = conet_ndis_main(argc, argv);

	co_debug_end();

	return ret;
}
