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

#include <colinux/user/debug.h>
#include <colinux/os/user/misc.h>
#include <colinux/common/common.h>
#include <colinux/user/macaddress.h>
#include "../daemon.h"
#include "pcap-registry.h"

COLINUX_DEFINE_MODULE("colinux-bridged-net-daemon");

/*******************************************************************************
 * Defines
 */
#define PCAP_NAME_HDR "\\Device\\NPF_"

/*******************************************************************************
 * Type Declarations
 */

typedef struct co_win32_overlapped {
	HANDLE handle;
	HANDLE read_event;
	HANDLE write_event;
	OVERLAPPED read_overlapped;
	OVERLAPPED write_overlapped;
	char buffer[0x10000];
	unsigned long size;
} co_win32_overlapped_t;

typedef struct co_win32_pcap {
	pcap_t *adhandle;
	struct pcap_pkthdr *pkt_header;
	u_char *buffer;
} co_win32_pcap_t;

/* Runtime paramters */

typedef struct start_parameters {
	bool_t show_help;
	bool_t mac_specified;
	char mac_address[18];
	bool_t name_specified;
	char interface_name[0x100];
	co_id_t instance;
	int index;
} start_parameters_t;

/*******************************************************************************
 * Globals 
 */
co_win32_pcap_t pcap_packet;
co_win32_overlapped_t daemon_overlapped;
start_parameters_t *daemon_parameters;

/*******************************************************************************
 * Write a packet to coLinux
 */
co_rc_t
co_win32_overlapped_write_async(co_win32_overlapped_t * overlapped,
				void *buffer, unsigned long size)
{
	unsigned long write_size;
	BOOL result;
	DWORD error;

	if (size == 0)
		return CO_RC(OK);

	co_debug_lvl(network, 12, "sending to daemon (0x%x size 0x%x)\n", buffer, size);
	result = WriteFile(overlapped->handle, buffer, size,
			   &write_size, &overlapped->write_overlapped);
	co_debug_lvl(network, 13, "packet sent (0x%x written)\n", write_size);

	if (!result) {
		switch (error = GetLastError()) {
		case ERROR_IO_PENDING:
			co_debug_lvl(network, 13, "pending\n");
			WaitForSingleObject(overlapped->write_event, INFINITE);
			co_debug_lvl(network, 14, "awakened\n");
			break;
		default:
			co_debug_lvl(network, 5, "daemon failed (%x)\n", error);
			return CO_RC(ERROR);
		}
	}

	return CO_RC(OK);
}

/*******************************************************************************
 * Take packet received from coLinux and retransmit using pcap.
 */
co_rc_t
co_win32_daemon_read_received(co_win32_overlapped_t * overlapped)
{
	int pcap_rc;
	/* Received packet from daemon. */
	co_message_t *message;
	message = (co_message_t *) overlapped->buffer;

	co_debug_lvl(network, 12, "sending to pcap (0x%x size 0x%x)\n", message->data, message->size);
	/* Send packet using pcap. */
	pcap_rc = pcap_sendpacket(pcap_packet.adhandle,
				  message->data, message->size);
	co_debug_lvl(network, 13, "sent (%x)\n", pcap_rc);

	return CO_RC(OK);
}

/*******************************************************************************
 * Take packet received from pcap and retransmit to coLinux.
 */
co_rc_t
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

	co_win32_overlapped_write_async(&daemon_overlapped, &message,
					sizeof (message));

	return CO_RC(OK);
}

/*******************************************************************************
 * Begin read call to coLinux, non-blocking.
 */
co_rc_t
co_win32_overlapped_read_async(co_win32_overlapped_t * overlapped)
{
	BOOL result;
	DWORD error;

	while (TRUE) {
		result = ReadFile(overlapped->handle,
				  &overlapped->buffer,
				  sizeof (overlapped->buffer),
				  &overlapped->size,
				  &overlapped->read_overlapped);

		if (!result) {
			error = GetLastError();
			switch (error) {
			case ERROR_IO_PENDING:
				return CO_RC(OK);

			default:
				co_debug_lvl(network, 5, "error: %x\n", error);
				return CO_RC(ERROR);
			}
		} else
			co_win32_daemon_read_received(overlapped);
	}

	return CO_RC(OK);
}

/*******************************************************************************
 * Handles completed read from coLinux, then starts next read.
 */
co_rc_t
co_win32_overlapped_read_completed(co_win32_overlapped_t * overlapped)
{
	BOOL result;

	result = GetOverlappedResult(overlapped->handle,
				     &overlapped->read_overlapped,
				     &overlapped->size, FALSE);

	if (result) {
		co_win32_daemon_read_received(overlapped);
		co_win32_overlapped_read_async(overlapped);
	} else {
		if (GetLastError() == ERROR_BROKEN_PIPE) {
			co_debug("Pipe broken, exiting\n");
			return CO_RC(ERROR);
		}

		co_debug("GetOverlappedResult error %d\n", GetLastError());
	}

	return CO_RC(OK);
}

/********************************************************************************
 * Initializes the data structure used to talk to coLinux
 */
void
co_win32_overlapped_init(co_win32_overlapped_t * overlapped, HANDLE handle)
{
	overlapped->handle = handle;
	overlapped->read_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	overlapped->write_event = CreateEvent(NULL, FALSE, FALSE, NULL);

	overlapped->read_overlapped.Offset = 0;
	overlapped->read_overlapped.OffsetHigh = 0;
	overlapped->read_overlapped.hEvent = overlapped->read_event;

	overlapped->write_overlapped.Offset = 0;
	overlapped->write_overlapped.OffsetHigh = 0;
	overlapped->write_overlapped.hEvent = overlapped->write_event;
}

/*******************************************************************************
 * The wait loop is run by the parent thread.
 * It handles packets going from the coLinux Daemon to winPCap.
 */
int
wait_loop(HANDLE daemon_handle)
{
	co_rc_t rc;
	ULONG status;

	co_win32_overlapped_init(&daemon_overlapped, daemon_handle);
	co_win32_overlapped_read_async(&daemon_overlapped);

	while (1) {
		// Attempt to receive packet from coLinux.
		status =
		    WaitForSingleObject(daemon_overlapped.read_event, INFINITE);
		switch (status) {
		case WAIT_OBJECT_0:	/* daemon */
			rc = co_win32_overlapped_read_completed(&daemon_overlapped);
			if (!CO_OK(rc))
				return 0;

			break;

		default:
			break;
		}
	}

	return 0;
}

/*******************************************************************************
 * The pcap2Daemon function is spawned as a thread.
 * It takes packets from winPCap and relays them to the coLinux Daemon.
 */
DWORD WINAPI
pcap2Daemon(LPVOID lpParam)
{
	int pcap_status;
	co_rc_t rc;

	while (1) {
		/* Attempt to receive packet from WPcap. */
		pcap_status = pcap_next_ex(pcap_packet.adhandle,
					   &pcap_packet.pkt_header,
					   &pcap_packet.buffer);
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
			co_debug_lvl(network, 5, "unexpected error reading from winPCap.\n");
			ExitProcess(0);
			return 0;
			break;
		}
	}

	/* We should never get to here. */
	co_debug_lvl(network, 5, "unexpected exit of winPCap read loop.\n");
	ExitProcess(0);
	return 0;
}

/*******************************************************************************
 * Get the Connection name for an PCAP Interface (using NetCfgInstanceId).
 */
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
			name_data,
			&len);
		if (status != ERROR_SUCCESS || name_type != REG_SZ) {
			co_terminal_print("Error opening registry key: %s\\%s\\%s",					NETWORK_CONNECTIONS_KEY, connection_string, name_string);
			return CO_RC(ERROR);
		}
		else {
			if (name_data) {
				snprintf(actual_name, actual_name_size, "%s", name_data);
			}
		}
		
		RegCloseKey(connection_key);
		
	}

	return CO_RC(OK);
}

/*******************************************************************************
 * Initialize winPCap interface.
 */
int
pcap_init()
{
	pcap_if_t *alldevs = NULL;
	pcap_if_t *device = NULL;
	pcap_if_t *found_device = NULL;
	int exit_code = 0;
	pcap_t *adhandle = NULL;
	char errbuf[PCAP_ERRBUF_SIZE];
	u_int netmask;
	char packet_filter[0x100];
	struct bpf_program fcode;

	co_snprintf(packet_filter, sizeof(packet_filter), 
		    "(ether dst %s) or (ether broadcast or multicast) or (ip broadcast or multicast)",
		    daemon_parameters->mac_address);

	/* Retrieve the device list */
	if (pcap_findalldevs(&alldevs, errbuf) == -1) {
		co_terminal_print("Error in pcap_findalldevs: %s\n", errbuf);
		exit_code = -1;
		goto pcap_out;
	}

	co_terminal_print("Looking for interface \"%s\"\n", daemon_parameters->interface_name);
	
	device = alldevs;
	char name_data[256];
	char connection_name_data[256];
	while (device) {
		
		memset(connection_name_data, 0, sizeof(connection_name_data));

		if (daemon_parameters->name_specified == PFALSE)
			break;

		snprintf(name_data, sizeof(name_data), "%s", device->name+(sizeof(PCAP_NAME_HDR) - 1));

		get_device_name(name_data, sizeof(name_data),
		                connection_name_data, sizeof(connection_name_data));

		if (strcmp(connection_name_data, "") != 0) {
			co_terminal_print("Checking connection: %s\n", connection_name_data);
			/*
			  Do an partial search, if partial search is found,
			   set this device as he found device, but continue
			   looping through devices.
			 */
			if (strstr(connection_name_data, daemon_parameters->interface_name) != NULL &&
			    found_device == NULL) {
				found_device = device;
			}
			
			/*
			 If an exact match exists, over-ride any found device,
			  setting exact match device to it.
			 */
			if (strcmp(connection_name_data, daemon_parameters->interface_name) == 0) {
				found_device = device;
			}
		}
		else {
			co_terminal_print("Adapter %s doesn't have a connection\n", device->description);
		}

		device = device->next;
	}

	if (found_device == NULL) {
		co_terminal_print("No matching adapter\n");
                exit_code = -1;
                goto pcap_out_close;
	}

	device = found_device;

	/* Open the first adapter. */
	if ((adhandle = pcap_open_live(device->name,	// name of the device
				       65536,	// captures entire packet.
				       1,	// promiscuous mode
				       1,	// read timeout
				       errbuf	// error buffer
	     )) == NULL) {
		co_terminal_print("Unable to open the adapter.\n");
		exit_code = -1;
		goto pcap_out_close;
	}

	/* Check the link layer. We support only Ethernet because coLinux
	 * only talks 802.3 so far.
	 */
	if (pcap_datalink(adhandle) != DLT_EN10MB) {
		co_terminal_print
		    ("This program works only on 802.3 Ethernet networks.\n");
		exit_code = -1;
		goto pcap_out_close;
	}

	if (device->addresses != NULL) {
		/* Retrieve the mask of the first address of the interface */
		netmask =
		    ((struct sockaddr_in *) (device->addresses->netmask))->sin_addr.
		    S_un.S_addr;
	} else {
		/* If the interface is without addresses we suppose to be in a C
		 * class network
		 */
		netmask = 0xffffff;
	}

	//compile the filter
	if (pcap_compile(adhandle, &fcode, packet_filter, 1, netmask) < 0) {
		co_terminal_print
		    ("Unable to compile the packet filter. Check the syntax.\n");
		exit_code = -1;
		goto pcap_out_close;
	}
	//set the filter
	if (pcap_setfilter(adhandle, &fcode) < 0) {
		co_terminal_print("Error setting the filter.\n");
		exit_code = -1;
		goto pcap_out_close;
	}

	co_terminal_print("Listening on: %s...\n", device->description);
	co_terminal_print("Listening for: %s\n", packet_filter);

	pcap_packet.adhandle = adhandle;

      pcap_out_close:
	// At this point, we don't need any more the device list. Free it
	pcap_freealldevs(alldevs);

      pcap_out:
	return exit_code;
}

/********************************************************************************
 * parameters
 */

void co_net_syntax()
{
	co_terminal_print("Cooperative Linux Bridged Network Daemon\n");
	co_terminal_print("Alejandro R. Sedeno, 2004 (c)\n");
	co_terminal_print("Dan Aloni, 2003-2004 (c)\n");
	co_terminal_print("\n");
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("  colinux-bridged-net-daemon -i index [-h] [-n 'adapter name'] [-mac xx:xx:xx:xx:xx:xx]\n");
	co_terminal_print("\n");
	co_terminal_print("    -h                      Show this help text\n");
	co_terminal_print("    -n 'adapter name'       The name of the network adapter to attach to\n");
	co_terminal_print("                            Without this option, the daemon tries to\n");
	co_terminal_print("                            guess which interface to use\n");
	co_terminal_print("    -i index                Network device index number (0 for eth0, 1 for\n");
	co_terminal_print("                            eth1, etc.)\n");
	co_terminal_print("    -c instance             coLinux instance ID to connect to\n");
	co_terminal_print("    -mac xx:xx:xx:xx:xx:xx  MAC address for the bridged interface\n");
}

static co_rc_t 
handle_paramters(start_parameters_t *start_parameters, int argc, char *argv[])
{
	char **param_scan = argv;
	const char *option;

	/* Default settings */
	start_parameters->index = -1;
	start_parameters->instance = -1;
	start_parameters->show_help = PFALSE;
	start_parameters->mac_specified = PFALSE;
	start_parameters->name_specified = PFALSE;

	/* Parse command line */
	while (*param_scan) {
		option = "-mac";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("Parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			co_snprintf(start_parameters->mac_address, 
				    sizeof(start_parameters->mac_address), 
				    "%s", *param_scan);

			start_parameters->mac_specified = PTRUE;
			param_scan++;
			continue;
		}

		option = "-i";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("Parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			sscanf(*param_scan, "%d", &start_parameters->index);
			param_scan++;
			continue;
		}

		option = "-c";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("Parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			sscanf(*param_scan, "%d", &start_parameters->instance);
			param_scan++;
			continue;
		}

		option = "-n";
		if (strcmp(*param_scan, option) == 0) {
			param_scan++;
			if (!(*param_scan)) {
				co_terminal_print("Parameter of command line option %s not specified\n", option);
				return CO_RC(ERROR);
			}

			co_snprintf(start_parameters->interface_name, 
				    sizeof(start_parameters->interface_name), 
				    "%s", *param_scan);

			start_parameters->name_specified = PTRUE;
			param_scan++;
			continue;
		}

		option = "-h";
		if (strcmp(*param_scan, option) == 0) {
			start_parameters->show_help = PTRUE;
		}
		param_scan++;
	}

	if (start_parameters->index == -1) {
		co_terminal_print("Device index not specified\n");
		return CO_RC(ERROR);
	}

	if ((start_parameters->index < 0) ||
	    (start_parameters->index >= CO_MODULE_MAX_CONET)) 
	{
		co_terminal_print("Invalid index: %d\n", start_parameters->index);
		return CO_RC(ERROR);
	}

	if (start_parameters->instance == -1) {
		co_terminal_print("coLinux instance not specificed\n");
		return CO_RC(ERROR);
	}

	return CO_RC(OK);	
}

int
conet_bridged_main(int argc, char *argv[])
{
	co_rc_t rc;
	HANDLE daemon_handle = 0;
	HANDLE pcap_thread;
	start_parameters_t start_parameters;
	int exit_code = 0;
	co_daemon_handle_t daemon_handle_;

	rc = handle_paramters(&start_parameters, argc, argv);
	if (!CO_OK(rc)) 
		return -1;

	if (start_parameters.show_help) {
		co_net_syntax();
		return 0;
	}

	if (!start_parameters.mac_specified) {
		co_terminal_print("Error, MAC address not specified\n");
		return CO_RC(ERROR);
	}

	if (start_parameters.index == -1) {
		co_terminal_print("Error, index not specified\n");
		return CO_RC(ERROR);
	}

	daemon_parameters = &start_parameters;

	exit_code = pcap_init();
	if (exit_code) {
		co_terminal_print("Error initializing winPCap\n");
		goto out;
	}

	rc = co_os_open_daemon_pipe(daemon_parameters->instance, 
				    CO_MODULE_CONET0 + daemon_parameters->index, &daemon_handle_);
	if (!CO_OK(rc)) {
		co_terminal_print("Error opening a pipe to the daemon\n");
		goto out;
	}

	pcap_thread = CreateThread(NULL, 0, pcap2Daemon, NULL, 0, NULL);
	if (pcap_thread == NULL) {
		co_terminal_print("Failed to spawn pcap_thread\n");
		goto out;
	}

	daemon_handle = daemon_handle_->handle;
	exit_code = wait_loop(daemon_handle);
	co_os_daemon_close(daemon_handle_);

      out:
	ExitProcess(exit_code);
	return exit_code;
}

/********************************************************************************
 * main...
 */

int
main(int argc, char *argv[])
{
	int ret;

	co_debug_start();
        
	ret = conet_bridged_main(argc, argv);

	co_debug_end();
    
	return ret;
}
