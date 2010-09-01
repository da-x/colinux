/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 * Alejandro R. Sedeno <asedeno@mit.edu>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>
#include <winsock2.h>
#include <pcap.h>

#include <stdio.h>

#include <colinux/common/common.h>
#include <colinux/user/debug.h>
#include <colinux/user/reactor.h>
#include <colinux/user/monitor.h>
#include <colinux/user/macaddress.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/current/user/reactor.h>
#include <colinux/os/current/user/misc.h>
#include "pcap-registry.h"

COLINUX_DEFINE_MODULE("colinux-bridged-net-daemon");

/*******************************************************************************
 * Defines
 */
#define PCAP_NAME_HDR "\\Device\\NPF_"

/*******************************************************************************
 * Type Declarations
 */

typedef struct co_win32_pcap {
	pcap_t *adhandle;
	struct pcap_pkthdr *pkt_header;
	u_char *buffer;
	char * dev_name;
	u_int netmask;
	volatile bool_t suspended;
	HANDLE pcap_thread;
	char packet_filter[0x100];
} co_win32_pcap_t;

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
co_win32_pcap_t pcap_packet;
co_reactor_t g_reactor = NULL;
co_user_monitor_t *g_monitor_handle = NULL;
start_parameters_t *daemon_parameters;

static co_rc_t monitor_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	co_message_t *message;
	unsigned long message_size;
	long size_left = size;
	long position = 0;
	int pcap_rc;

	while (size_left > 0) {
		message = (typeof(message))(&buffer[position]);
		message_size = message->size + sizeof(*message);
		size_left -= message_size;
		if (size_left >= 0) {
			pcap_rc = pcap_sendpacket(pcap_packet.adhandle,
						  message->data, message->size);
			/* TODO */
		}
		position += message_size;
	}

	return CO_RC(OK);
}

/*******************************************************************************
 * Take packet received from pcap and retransmit to coLinux.
 */

static co_rc_t
co_win32_pcap_read_received(co_win32_pcap_t * pcap_pkt)
{
	struct {
		co_message_t message;
		co_linux_message_t linux;
		char data[pcap_pkt->pkt_header->len];
	} message;

	message.message.from = CO_MODULE_CONET0 + daemon_parameters->index;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof (message) - sizeof (message.message);
	message.linux.device = CO_DEVICE_NETWORK;
	message.linux.unit = daemon_parameters->index;
	message.linux.size = pcap_pkt->pkt_header->len;
	memcpy(message.data, pcap_pkt->buffer, pcap_pkt->pkt_header->len);

	g_monitor_handle->reactor_user->send(g_monitor_handle->reactor_user,
					     (unsigned char *)&message, sizeof(message));

	return CO_RC(OK);
}


/*******************************************************************************
 * The pcap2Daemon function is spawned as a thread.
 * It takes packets from winPCap and relays them to the coLinux Daemon.
 */
static DWORD WINAPI
pcap2Daemon(LPVOID lpParam)
{
	int pcap_status;
	co_rc_t rc;

	while (1) {
		/* Attempt to receive packet from WPcap. */
		pcap_status = pcap_next_ex(pcap_packet.adhandle,
					   &pcap_packet.pkt_header,
					   (const u_char **)&pcap_packet.buffer);
		switch (pcap_status) {
		case 1:	/* Packet read */
			rc = co_win32_pcap_read_received(&pcap_packet);
			if (!CO_OK(rc))
				return 0;
			break;

		case 0:	/* Timeout  */
			break;

		default:
			/* Error or EOF(offline capture only) */
			/* Typically: Network adapter disabled or in power off mode (Vista suspend) */
			co_debug_lvl(network, 5, "error %d reading from winPCap: %s",
				     pcap_status, pcap_geterr(pcap_packet.adhandle));

			co_debug_lvl(network, 3, "Thread stopped.");
			pcap_packet.suspended = TRUE;
			SuspendThread(pcap_packet.pcap_thread);
			pcap_packet.suspended = FALSE;
			co_debug_lvl(network, 3, "Thread continued.");
		}
	}

	/* We should never get to here. */
	return 0;
}

/*******************************************************************************
 * Get the Connection name for an PCAP Interface (using NetCfgInstanceId).
 */
static
co_rc_t get_device_name(char *name,
                        int name_size,
			char *actual_name,
			int actual_name_size)
{
	char connection_string[256];
	HKEY connection_key;
	char name_data[256];
	DWORD name_type;
	const char name_string[] = "Name";
	LONG status;
	DWORD len;

	snprintf(connection_string,
	         sizeof(connection_string),
		 "%s\\%s\\Connection",
		 NETWORK_CONNECTIONS_KEY, name);

        status = RegOpenKeyEx(
	         HKEY_LOCAL_MACHINE,
		 connection_string,
		 0,
		 KEY_READ,
		 &connection_key);

        if (status == ERROR_SUCCESS) {
		len = sizeof(name_data);
		status = RegQueryValueEx(
			connection_key,
			name_string,
			NULL,
			&name_type,
			(PBYTE)name_data,
			&len);
		if (status != ERROR_SUCCESS || name_type != REG_SZ) {
			co_terminal_print("conet-bridged-daemon: error opening registry key: %s\\%s\\%s",
					  NETWORK_CONNECTIONS_KEY, connection_string, name_string);
			return CO_RC(ERROR);
		}
		else {
			snprintf(actual_name, actual_name_size, "%s", name_data);
		}

		RegCloseKey(connection_key);

	}

	return CO_RC(OK);
}

static co_rc_t co_pcap_search(void)
{
	pcap_if_t *alldevs = NULL;
	pcap_if_t *device;
	pcap_if_t *found_device = NULL;
	char errbuf[PCAP_ERRBUF_SIZE];

	/* Retrieve the device list */
	if (pcap_findalldevs(&alldevs, errbuf) == -1) {
		co_terminal_print("conet-bridged-daemon: error in pcap_findalldevs: %s\n", errbuf);
		return CO_RC(ERROR);
	}

	if (daemon_parameters->name_specified == PTRUE)
		co_terminal_print("conet-bridged-daemon: Looking for interface \"%s\"\n", daemon_parameters->interface_name);
	else
		co_terminal_print("conet-bridged-daemon: Auto selecting name for bridged interface\n");

	device = alldevs;
	char name_data[256];
	char connection_name_data[256];
	while (device) {

		memset(connection_name_data, 0, sizeof(connection_name_data));

		snprintf(name_data, sizeof(name_data), "%s", device->name+(sizeof(PCAP_NAME_HDR) - 1));

		get_device_name(name_data, sizeof(name_data),
		                connection_name_data, sizeof(connection_name_data));

		if (*connection_name_data != 0) {
			co_terminal_print("conet-bridged-daemon: checking connection '%s'\n", connection_name_data);

			if (daemon_parameters->name_specified == PTRUE) {
				/*
				 If an exact match exists, over-ride any found device,
				  setting exact match device to it.
				 */
				if (strcmp(connection_name_data, daemon_parameters->interface_name) == 0) {
					found_device = device;
					break;
				}

				/*
				  Do an partial search, if partial search is found,
				   set this device as he found device, but continue
				   looping through devices.
				 */

				if (found_device == NULL &&
				    strstr(connection_name_data, daemon_parameters->interface_name) != NULL) {
					found_device = device;
				}
			} else {
				/*
				 If no name specified and network has an address,
				  autoselect first device.
				*/
				if (device->addresses) {
					found_device = device;
					break;
				}
			}

		}
		else {
			co_terminal_print("conet-bridged-daemon: adapter %s doesn't have a connection\n", device->description);
		}

		device = device->next;
	}

	if (found_device == NULL) {
		co_terminal_print("conet-bridged-daemon: no matching adapter\n");
		pcap_freealldevs(alldevs);
		return CO_RC(ERROR);
	}

	device = found_device;
	pcap_packet.dev_name = strdup(device->name);

	if (device->addresses != NULL) {
		/* Retrieve the mask of the first address of the interface */
		pcap_packet.netmask = ((struct sockaddr_in *) \
			(device->addresses->netmask))->sin_addr.S_un.S_addr;
	} else {
		/* If the interface is without addresses we suppose to be in a C
		 * class network
		 */
		pcap_packet.netmask = 0xffffff;
	}

	co_snprintf(pcap_packet.packet_filter, sizeof(pcap_packet.packet_filter),
		    "(ether dst %s) or (multicast and not ether src %s)",
		    daemon_parameters->mac_address, daemon_parameters->mac_address);

	co_terminal_print("conet-bridged-daemon: listening on: %s...\n", device->description);
	co_terminal_print("conet-bridged-daemon: Filter rule: %s\n", pcap_packet.packet_filter);
	if (daemon_parameters->promisc == 0)	// promiscuous mode?
		co_terminal_print("conet-bridged-daemon: Promiscuous mode disabled\n");

	// At this point, we don't need any more the device list. Free it
	pcap_freealldevs(alldevs);

	return CO_RC(OK);
}

static co_rc_t co_pcap_open(void)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	struct bpf_program fcode;

	/* Open the adapter. */
	if ((pcap_packet.adhandle = pcap_open_live(pcap_packet.dev_name,	// name of the device
				       65536,	// captures entire packet.
				       daemon_parameters->promisc,	// promiscuous mode
				       1,	// read timeout
				       errbuf	// error buffer
	     )) == NULL) {
		co_debug("conet-bridged-daemon: unable to open the adapter.");
		return CO_RC(ERROR);
	}

	/* Check the link layer. We support only Ethernet because coLinux
	 * only talks 802.3 so far.
	 */
	if (pcap_datalink(pcap_packet.adhandle) != DLT_EN10MB) {
		co_terminal_print("conet-bridged-daemon: this program works only on 802.3 Ethernet networks.\n");
		return CO_RC(ERROR);
	}

	//compile the filter
	if (pcap_compile(pcap_packet.adhandle, &fcode, pcap_packet.packet_filter, 1, pcap_packet.netmask) < 0) {
		co_terminal_print("conet-bridged-daemon: unable to compile the packet filter. Check the syntax.\n");
		return CO_RC(ERROR);
	}

	//set the filter
	if (pcap_setfilter(pcap_packet.adhandle, &fcode) < 0) {
		co_terminal_print("conet-bridged-daemon: error setting the filter.\n");
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

static void co_pcap_close(void)
{
	if (pcap_packet.adhandle) {
		pcap_close(pcap_packet.adhandle);
		pcap_packet.adhandle = NULL;
	}
}


/*******************************************************************************
 * Initialize winPCap interface.
 */
static co_rc_t co_pcap_init(void)
{
	co_rc_t rc;

	rc = co_pcap_search();
	if (!CO_OK(rc))
		return rc;

	return co_pcap_open();
}

/********************************************************************************
 * parameters
 */

static void co_net_syntax(void)
{
	co_terminal_print("Cooperative Linux Bridged Network Daemon\n");
	co_terminal_print("Alejandro R. Sedeno, 2004 (c)\n");
	co_terminal_print("Dan Aloni, 2003-2004 (c)\n");
	co_terminal_print("\n");
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("  colinux-bridged-net-daemon -i pid -u unit [-h] [-n 'adapter name'] [-mac xx:xx:xx:xx:xx:xx] [-p promiscuous]\n");
	co_terminal_print("\n");
	co_terminal_print("    -h                      Show this help text\n");
	co_terminal_print("    -i pid                  coLinux instance ID to connect to\n");
	co_terminal_print("    -u unit                 Network device index number (0 for eth0, 1 for\n");
	co_terminal_print("                            eth1, etc.)\n");
	co_terminal_print("    -n 'adapter name'       The name of the network adapter to attach to\n");
	co_terminal_print("                            Without this option, the daemon tries to\n");
	co_terminal_print("                            guess which interface to use\n");
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
				co_terminal_print("conet-bridged-daemon: parameter of command line option %s not specified\n", option);
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
				co_terminal_print("conet-bridged-daemon: parameter of command line option %s not specified\n", option);
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
				co_terminal_print("conet-bridged-daemon: parameter of command line option %s not specified\n", option);
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
				co_terminal_print("conet-bridged-daemon: parameter of command line option %s not specified\n", option);
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
				co_terminal_print("conet-bridged-daemon: parameter of command line option %s not specified\n", option);
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
		co_terminal_print("conet-bridged-daemon: device index not specified\n");
		return CO_RC(ERROR);
	}

	if ((start_parameters->index < 0) ||
	    (start_parameters->index >= CO_MODULE_MAX_CONET))
	{
		co_terminal_print("conet-bridged-daemon: invalid index: %d\n", start_parameters->index);
		return CO_RC(ERROR);
	}

	if (start_parameters->instance == -1) {
		co_terminal_print("conet-bridged-daemon: coLinux instance not specificed\n");
		return CO_RC(ERROR);
	}

	if (!start_parameters->mac_specified) {
		co_terminal_print("conet-bridged-daemon: error, MAC address not specified\n");
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

static co_rc_t conet_bridged_main(int argc, char *argv[])
{
	co_rc_t rc;
	start_parameters_t start_parameters;
	co_module_t modules[] = {CO_MODULE_CONET0, };

	rc = handle_parameters(&start_parameters, argc, argv);
	if (!CO_OK(rc))
		return rc;

	if (start_parameters.show_help) {
		co_net_syntax();
		return CO_RC(OK);
	}

	daemon_parameters = &start_parameters;

	rc = co_pcap_init();
	if (!CO_OK(rc)) {
		co_terminal_print("conet-bridged-daemon: error initializing winPCap\n");
		goto out;
	}

	rc = co_reactor_create(&g_reactor);
	if (!CO_OK(rc))
		goto out;

	modules[0] += start_parameters.index;
	rc = co_user_monitor_open(g_reactor, monitor_receive,
				  start_parameters.instance, modules,
				  sizeof(modules)/sizeof(co_module_t),
				  &g_monitor_handle);
	if (!CO_OK(rc))
		goto out;

	pcap_packet.pcap_thread = CreateThread(NULL, 0, pcap2Daemon, NULL, 0, NULL);
	if (!pcap_packet.pcap_thread) {
		co_terminal_print("conet-bridged-daemon: failed to spawn pcap_thread\n");
		rc = CO_RC(ERROR);
		goto out;
	}

	while (1) {
		rc = co_reactor_select(g_reactor, -1);
		if (!CO_OK(rc))
			goto out;

		if (pcap_packet.suspended) {
			DWORD retcode;
			int loop;

			co_terminal_print("conet-bridged-daemon: offline\n");
			co_pcap_close();

			// Try to open adapter again after loosing connection.
			// Pause 0 ... 19 seconds between every try (total 190 sec).
			for (loop = 0; loop < 20; loop++) {

				// Check, that thread exist and is waiting
				if (!GetExitCodeThread(pcap_packet.pcap_thread, &retcode)) {
					co_terminal_print_last_error("conet-bridged-daemon: thread dead");
					return CO_RC(ERROR);
				}

				if (retcode != STILL_ACTIVE) {
					co_terminal_print("conet-bridged-daemon: thread dead (%ld)\n", retcode);
					return CO_RC(ERROR);
				}

				Sleep(1000 * loop);

				co_debug("reinitializing winPCap (%d)\n", loop);
				rc = co_pcap_open();
				if (CO_OK(rc))
					break;
			}

			if (!CO_OK(rc)) {
				co_terminal_print("conet-bridged-daemon: error reinitializing winPCap\n");
				goto out;
			}

			// Continue receiver thread
			if (ResumeThread(pcap_packet.pcap_thread) == -1) {
				co_terminal_print_last_error("conet-bridged-daemon: failed to resume thread");
				rc = CO_RC(ERROR);
				goto out;
			}

			co_terminal_print("conet-bridged-daemon: online\n");
		}
	}

out:
	co_pcap_close();

	return rc;
}

/********************************************************************************
 * main...
 */

int
main(int argc, char *argv[])
{
	co_rc_t rc;

	co_debug_start();
	co_process_high_priority_set();

	rc = conet_bridged_main(argc, argv);

	co_debug("exit (rc %x)", (int)rc);
	co_debug_end();

	return (CO_OK(rc)) ? 0 : -1;
}
