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

// missed NDIS event API
VOID NdisInitializeEvent(
	IN PNDIS_EVENT		Event
	);

BOOLEAN NdisWaitEvent(
	IN PNDIS_EVENT		Event,
	IN UINT			MsToWait
	); 

VOID NdisSetEvent(
	IN PNDIS_EVENT		Event
	);

VOID NdisResetEvent(
	IN PNDIS_EVENT		Event
	);

conet_adapter_t *co_conet_create_adapter(
	co_monitor_t		*monitor, 
	int			conet_unit
	);

void co_conet_free_adapter(
	conet_adapter_t		*adapter
	);

void DDKAPI co_conet_proto_bind_adapter(
	OUT PNDIS_STATUS	Status,
	IN  NDIS_HANDLE		BindContext,
	IN  PNDIS_STRING	DeviceName,
	IN  PVOID		SystemSpecific1,
	IN  PVOID		SystemSpecific2
	);

void DDKAPI co_conet_proto_unbind_adapter(
	OUT PNDIS_STATUS	Status,
	IN  NDIS_HANDLE		ProtocolBindingContext,
	IN  NDIS_HANDLE		UnbindContext
	);

void DDKAPI co_conet_proto_open_adapter_complete(
	IN NDIS_HANDLE		ProtocolBindingContext,
	IN NDIS_STATUS		Status,
	IN NDIS_STATUS		OpenErrorStatus
	);

void DDKAPI co_conet_proto_close_adapter_complete(
	IN NDIS_HANDLE		ProtocolBindingContext,
	IN NDIS_STATUS		Status
	);

NDIS_STATUS DDKAPI co_conet_proto_receive(
	IN NDIS_HANDLE		ProtocolBindingContext,
	IN NDIS_HANDLE		MacReceiveContext,
	IN PVOID		HeaderBuffer,
	IN UINT			HeaderBufferSize,
	IN PVOID		LookAheadBuffer,
	IN UINT			LookAheadBufferSize,
	IN UINT			PacketSize
	);

void DDKAPI co_conet_proto_receive_complete(
	IN NDIS_HANDLE		ProtocolBindingContext
	);

void DDKAPI co_conet_proto_request_complete(
	IN NDIS_HANDLE		ProtocolBindingContext,
	IN PNDIS_REQUEST	NdisRequest,
	IN NDIS_STATUS		Status
	);

void DDKAPI co_conet_proto_send_complete(
	IN NDIS_HANDLE		ProtocolBindingContext,
	IN PNDIS_PACKET		Packet,
	IN NDIS_STATUS		Status
	);

void DDKAPI co_conet_proto_reset_complete(
	IN NDIS_HANDLE		ProtocolBindingContext,
	IN NDIS_STATUS		Status
	);

void DDKAPI co_conet_proto_status(
	IN NDIS_HANDLE		ProtocolBindingContext,
	IN NDIS_STATUS		Status,
	IN PVOID		StatusBuffer,
	IN UINT			StatusBufferSize
	);

void DDKAPI co_conet_proto_status_complete(
	IN NDIS_HANDLE		ProtocolBindingContext
	);

void DDKAPI co_conet_proto_transfer_complete(
	IN NDIS_HANDLE		ProtocolBindingContext,
	IN PNDIS_PACKET		Packet,
	IN NDIS_STATUS		Status,
	IN UINT			BytesTransferred
	);

int DDKAPI co_conet_proto_receive_packet(
	IN NDIS_HANDLE		ProtocolBindingContext,
	IN PNDIS_PACKET		Packet
	);
   
NDIS_STATUS DDKAPI co_conet_proto_pnp_handler(
	IN NDIS_HANDLE		ProtocolBindingContext,
	IN PNET_PNP_EVENT	pNetPnPEvent
	);

#endif
