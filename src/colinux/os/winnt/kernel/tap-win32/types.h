/*
 *  TAP-Win32 -- A kernel driver to provide virtual tap device functionality
 *               on Windows.  Originally derived from the CIPE-Win32
 *               project by Damion K. Wilson, with extensive modifications by
 *               James Yonan.
 *
 *  All source code which derives from the CIPE-Win32 project is
 *  Copyright (C) Damion K. Wilson, 2003, and is released under the
 *  GPL version 2 (see below).
 *
 *  All other source code is Copyright (C) James Yonan, 2003,
 *  and is released under the GPL version 2 (see below).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program (see the file COPYING included with this
 *  distribution); if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef TAP_TYPES_DEFINED
#define TAP_TYPES_DEFINED

typedef struct _Queue
{
  ULONG base;
  ULONG size;
  ULONG capacity;
  ULONG max_size;
  PVOID data[];
} Queue;

typedef struct _TapAdapter;
typedef struct _TapPacket;

typedef union _TapAdapterQuery
   {
    NDIS_HARDWARE_STATUS m_HardwareStatus;
    NDIS_MEDIUM m_Medium;
    UCHAR m_MacAddress [6];
    UCHAR m_Buffer [256];
    ULONG m_Long;
    USHORT m_Short;
    UCHAR m_Byte;
   }
TapAdapterQuery, *TapAdapterQueryPointer;

typedef struct _TapExtension
   {
     struct _TapAdapter *m_Adapter;
     Queue *m_PacketQueue, *m_IrpQueue;
     BOOLEAN m_halt;
   }
TapExtension, *TapExtensionPointer;


typedef struct _TapPacket
   {
#   define TAP_PACKET_SIZE(data_size) (sizeof (TapPacket) + (data_size))
#   define TP_POINT_TO_POINT 0x80000000
#   define TP_SIZE_MASK      (~TP_POINT_TO_POINT)
    ULONG m_SizeFlags;
    UCHAR m_Data []; // m_Data must be the last struct member
   }
TapPacket, *TapPacketPointer;

typedef struct _TapAdapter
{
  MACADDR m_MAC;
  BOOLEAN m_TapIsRunning, m_InterfaceIsRunning;
  unsigned char *m_Name, *m_TapName;
  NDIS_HANDLE m_MiniportAdapterHandle;
  LONG m_Rx, m_Tx, m_RxErr, m_TxErr;
  UNICODE_STRING m_UnicodeLinkName;
  PDEVICE_OBJECT m_TapDevice;
  NDIS_SPIN_LOCK m_Lock;
  NDIS_SPIN_LOCK m_QueueLock;
  NDIS_MEDIUM m_Medium;
  ULONG m_Lookahead;
  ULONG m_TapOpens;
  ULONG m_MTU;

  // TRUE if device should always be
  // "connected" even when device node
  // is not open by a userspace process.
  BOOLEAN m_MediaStateAlwaysConnected;

  // TRUE if device is "connected"
  BOOLEAN m_MediaState;

  // Info for point-to-point mode
  BOOLEAN m_PointToPoint;
  IPADDR m_localIP;
  IPADDR m_remoteIP;
  ETH_HEADER m_TapToUser;
  ETH_HEADER m_UserToTap;
  MACADDR m_MAC_Broadcast;

  // Used for device status ioctl only
  const char *m_LastErrorFilename;
  int m_LastErrorLineNumber;
  LONG m_NumTapOpens;
  char m_DeviceState;

  // Help to tear down the adapter by keeping
  // some state information on allocated
  // resources.
  BOOLEAN m_CalledAdapterFreeResources;
  BOOLEAN m_CalledTapDeviceFreeResources;
  BOOLEAN m_RegisteredAdapterShutdownHandler;
  BOOLEAN m_AllocatedSpinlocks;
  BOOLEAN m_DeviceExtensionIsAccessible;
  BOOLEAN m_CreatedSymbolLink;
  BOOLEAN m_CreatedUnicodeLinkName;

} TapAdapter, *TapAdapterPointer;

#endif
