/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003-2004 (c)
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
#include "../daemon.h"

/*
 * IMPORTANT NOTE:
 *
 * This is work-in-progress. This daemon is currently hardcoded
 * to work against coLinux0 as conet0. Expect changes.
 *
 */

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

/*******************************************************************************
 * Globals 
 */
co_win32_pcap_t pcap_packet;
co_win32_overlapped_t daemon_overlapped;

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

	result = WriteFile(overlapped->handle, buffer, size,
			   &write_size, &overlapped->write_overlapped);

	if (!result) {
		switch (error = GetLastError()) {
		case ERROR_IO_PENDING:
			WaitForSingleObject(overlapped->write_event, INFINITE);
			break;
		default:
			co_debug("packet tx: daemon - FAILED");
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

	/* Send packet using pcap. */
	pcap_rc = pcap_sendpacket(pcap_packet.adhandle,
				  message->data, message->size);

	if (pcap_rc)
		co_debug("packet tx: pcap - FAILED\n");

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

	message.message.from = CO_MODULE_CONET0;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof (message) - sizeof (message.message);
	message.linux.device = CO_DEVICE_NETWORK;
	message.linux.unit = 0;
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
				co_debug("Error: %x\n", error);
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
 * The wait loop is run by the parent theread.
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
			rc = co_win32_overlapped_read_completed
			    (&daemon_overlapped);
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
		// Attempt to receive packet from WPcap.
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
			co_debug("Unexpected error reading from winPCap.\n");
			ExitProcess(0);
			return 0;
			break;
		}
	}

	// We should never get to here.
	co_debug("Unexpected exit of winPCap read loop.\n");
	ExitProcess(0);
	return 0;
}

/*******************************************************************************
 * Initialize winPCap interface.
 */
int
pcap_init()
{
	pcap_if_t *alldevs;
	pcap_if_t *d;
	int exit_code = 0;
	pcap_t *adhandle;
	char errbuf[PCAP_ERRBUF_SIZE];
	char TAPDevice[] =
	    "\\Device\\NPF_{21DB0FAC-2EB2-4F70-A0E0-83FB1D29EB9C}";
	u_int netmask;

	/* TODO: Read MAC from config file. For now it's "\0CONE0" */
	char packet_filter[] =
	    "(ether dst 00:43:4F:4E:45:30) or (ether broadcast or multicast) or (ip broadcast or multicast)";
	struct bpf_program fcode;

	/* Retrieve the device list */
	if (pcap_findalldevs(&alldevs, errbuf) == -1) {
		co_debug("Error in pcap_findalldevs: %s\n", errbuf);
		exit_code = -1;
		goto pcap_out;
	}

	d = alldevs;
	while (1) {
		if (!(strcmp(d->name, TAPDevice) == 0  || 
		      strstr(d->description, "VMware") != NULL))
		{
			break;
		}
		d = d->next;
	}

	/* Open the first adapter. */
	/* TODO: Add clever network interface auto-selector. */
	/* TODO: Add network interface selection in config file. */
	if ((adhandle = pcap_open_live(d->name,	// name of the device
				       65536,	// captures entire packet.
				       1,	// promiscuous mode
				       1,	// read timeout
				       errbuf	// error buffer
	     )) == NULL) {
		co_debug("Unable to open the adapter.\n");
		exit_code = -1;
		goto pcap_out_close;
	}

	/* Check the link layer. We support only Ethernet because coLinux
	 * only talks 802.3 so far.
	 */
	if (pcap_datalink(adhandle) != DLT_EN10MB) {
		co_debug
		    ("This program works only on 802.3 Ethernet networks.\n");
		exit_code = -1;
		goto pcap_out_close;
	}

	if (d->addresses != NULL) {
		/* Retrieve the mask of the first address of the interface */
		netmask =
		    ((struct sockaddr_in *) (d->addresses->netmask))->sin_addr.
		    S_un.S_addr;
	} else {
		/* If the interface is without addresses we suppose to be in a C
		 * class network
		 */
		netmask = 0xffffff;
	}

	//compile the filter
	if (pcap_compile(adhandle, &fcode, packet_filter, 1, netmask) < 0) {
		co_debug
		    ("Unable to compile the packet filter. Check the syntax.\n");
		exit_code = -1;
		goto pcap_out_close;
	}
	//set the filter
	if (pcap_setfilter(adhandle, &fcode) < 0) {
		co_debug("Error setting the filter.\n");
		exit_code = -1;
		goto pcap_out_close;
	}

	co_debug("pcap-daemon: Listening on: %s...\n", d->description);
	co_debug("pcap-daemon: Listening for: %s\n", packet_filter);

	pcap_packet.adhandle = adhandle;

      pcap_out_close:
	// At this point, we don't need any more the device list. Free it
	pcap_freealldevs(alldevs);

      pcap_out:
	return exit_code;
}

/********************************************************************************
 * main...
 */
int
main(int argc, char *argv[])
{
	co_rc_t rc;
	HANDLE daemon_handle = 0;
	HANDLE pcap_thread;
	int exit_code = 0;
	co_daemon_handle_t daemon_handle_;

	exit_code = pcap_init();
	if (exit_code) {
		co_debug("Error Initializing winPCap\n");
		goto out;
	}

	rc = co_os_open_daemon_pipe(0, CO_MODULE_CONET0, &daemon_handle_);
	if (!CO_OK(rc)) {
		co_debug("Error opening a pipe to the daemon\n");
		goto out;
	}

	pcap_thread = CreateThread(NULL, 0, pcap2Daemon, NULL, 0, NULL);

	if (pcap_thread == NULL) {
		co_debug("Failed to spawn pcap_thread");
		goto out;
	}

	daemon_handle = daemon_handle_->handle;
	exit_code = wait_loop(daemon_handle);
	co_os_daemon_close(daemon_handle_);

      out:
	ExitProcess(exit_code);
	return exit_code;
}
