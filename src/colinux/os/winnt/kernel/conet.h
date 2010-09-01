/*
 * This source code is a part of coLinux source package.
 *
 * Ligong Liu <liulg@mail.reachstone.com>, 2008 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef _CO_CONET_H_
#define _CO_CONET_H_

#define CONET_MAX_PACKET_SIZE		1514
#define CONET_MAX_LOOKASIDE_SIZE	54
#define CONET_MAX_PACKET_DESCRIPTOR	128
#define CONET_MAX_PACKET_BUFFER		(2*CONET_MAX_PACKET_DESCRIPTOR)

typedef struct _conet_packet {
	NDIS_PACKET	ndis_packet;		/* must be the first member! */
	co_list_t	list_node;
	NDIS_STATUS	transfer_status;
	UINT		bytes_transferred;
	UCHAR		packet_buffer[CONET_MAX_PACKET_SIZE];
} conet_packet_t;

typedef struct _conet_adapter {
	co_list_t	list_node;		/* list link to osdep conet_adapters list */
	co_monitor_t	*monitor;		/* the monitor that opened this adapter */
	int		conet_unit;		/* colinux conet unit id */
	int		promisc;		/* true if works in promisc mode */
	long		packet_filter;		/* NDIS packet filter */
	char		macaddr[6];		/* MAC address of conet adapter */
	NDIS_HANDLE	binding_handle; 	/* ndis binding handle */
	NDIS_STATUS	binding_status;		/* binding result status */
	NDIS_EVENT	binding_event;		/* binding notification event */
	NDIS_HANDLE	buffer_pool;		/* packet buffer pool */
	NDIS_HANDLE	packet_pool;		/* packet pool */
	NDIS_STATUS	general_status;		/* adapter current status */
	NDIS_SPIN_LOCK	pending_transfer_lock;
	co_list_t	pending_transfer_list;	/* lists of conet_packet pending on ndis data transfer */
	NDIS_SPIN_LOCK	pending_send_lock;
	co_list_t	pending_send_list;	/* lists of conet_packet pending on ndis send */
} conet_adapter_t;

typedef struct _conet_binding_context {
	co_monitor_t	*monitor;
	conet_adapter_t	*adapter;
} conet_binding_context;

typedef struct _conet_message_transfer_context {
	co_monitor_t	*monitor;
	co_message_t	*message;
	PIO_WORKITEM	work_item;
} conet_message_transfer_context_t;

#define ETH_P_IP	0x0800		/* Internet Protocol packet	*/
#define ETH_P_ARP	0x0806		/* Address Resolution packet */

typedef struct _ETHER_HDR {
	unsigned char	h_dest[6];	/* destination eth addr	*/
	unsigned char	h_source[6];	/* source ether addr	*/
	unsigned short	h_proto;		/* packet type ID field	*/
} ETHER_HDR, *PETHER_HDR;

#endif
