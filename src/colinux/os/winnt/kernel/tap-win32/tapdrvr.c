/*
 *  TAP-Win32 -- A kernel driver to provide virtual tap device
 *               functionality on Windows.  Originally derived
 *               from the CIPE-Win32 project by Damion K. Wilson,
 *               with extensive modifications by James Yonan.
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

//======================================================
// This driver is designed to work on Win 2000 or higher
// versions of Windows.
//
// It is SMP-safe and handles NDIS 5 power management.
//
// By default we operate as a "tap" virtual ethernet
// 802.3 interface, but we can emulate a "tun"
// interface (point-to-point IPv4) through the
// TAP_IOCTL_CONFIG_POINT_TO_POINT ioctl.
//======================================================

#define NDIS_MINIPORT_DRIVER
#define BINARY_COMPATIBLE 0
#define NDIS50_MINIPORT 1
#define NDIS_WDM 0
#define NDIS50 1
#define NTSTRSAFE_LIB

#include <ndis.h>
#include <ntstrsafe.h>

#include "constants.h"
#include "types.h"
#include "prototypes.h"
#include "error.h"
#include "endian.h"

#include "mem.c"
#include "macinfo.c"
#include "error.c"

//========================================================
//                            Globals
//========================================================
PDRIVER_DISPATCH g_DispatchHook[IRP_MJ_MAXIMUM_FUNCTION + 1];
NDIS_MINIPORT_CHARACTERISTICS g_Properties;
PDRIVER_OBJECT g_TapDriverObject = NULL;
char g_DispatchFunctionsHooked = 0;
NDIS_HANDLE g_NdisWrapperHandle;
MACADDR g_MAC = { 0, 0, 0, 0, 0, 0 };

#define IS_UP(a) \
  ((a)->m_TapIsRunning && (a)->m_InterfaceIsRunning)

#define INCREMENT_STAT(s) ++(s)

UINT g_SupportedOIDList[] = {
  OID_GEN_HARDWARE_STATUS,
  OID_GEN_MEDIA_SUPPORTED,
  OID_GEN_MEDIA_IN_USE,
  OID_GEN_MAXIMUM_LOOKAHEAD,
  OID_GEN_MAC_OPTIONS,
  OID_GEN_LINK_SPEED,
  OID_GEN_TRANSMIT_BLOCK_SIZE,
  OID_GEN_RECEIVE_BLOCK_SIZE,
  OID_GEN_VENDOR_DESCRIPTION,
  OID_GEN_DRIVER_VERSION,
  OID_GEN_XMIT_OK,
  OID_GEN_RCV_OK,
  OID_GEN_XMIT_ERROR,
  OID_GEN_RCV_ERROR,
  OID_802_3_PERMANENT_ADDRESS,
  OID_802_3_CURRENT_ADDRESS,
  OID_GEN_RCV_NO_BUFFER,
  OID_802_3_RCV_ERROR_ALIGNMENT,
  OID_802_3_XMIT_ONE_COLLISION,
  OID_802_3_XMIT_MORE_COLLISIONS,
  OID_802_3_MULTICAST_LIST,
  OID_802_3_MAXIMUM_LIST_SIZE,
  OID_GEN_VENDOR_ID,
  OID_GEN_CURRENT_LOOKAHEAD,
  OID_GEN_CURRENT_PACKET_FILTER,
  OID_GEN_PROTOCOL_OPTIONS,
  OID_GEN_MAXIMUM_TOTAL_SIZE,
  OID_GEN_TRANSMIT_BUFFER_SPACE,
  OID_GEN_RECEIVE_BUFFER_SPACE,
  OID_GEN_MAXIMUM_FRAME_SIZE,
  OID_GEN_VENDOR_DRIVER_VERSION,
  OID_GEN_MAXIMUM_SEND_PACKETS,
  OID_GEN_MEDIA_CONNECT_STATUS,
  OID_GEN_SUPPORTED_LIST
};

//============================================================
//                         Driver Entry
//============================================================
#pragma NDIS_INIT_FUNCTION (DriverEntry)

NTSTATUS
DriverEntry (IN PDRIVER_OBJECT p_DriverObject,
	     IN PUNICODE_STRING p_RegistryPath)
{
  NDIS_STATUS l_Status = NDIS_STATUS_FAILURE;

  //========================================================
  // Notify NDIS that a new miniport driver is initializing.
  //========================================================

  NdisMInitializeWrapper (&g_NdisWrapperHandle,
			  g_TapDriverObject = p_DriverObject,
			  p_RegistryPath, NULL);

  //=======================================
  // Set and register miniport entry points
  //=======================================

  NdisZeroMemory (&g_Properties, sizeof (g_Properties));

  g_Properties.MajorNdisVersion = TAP_NDIS_MAJOR_VERSION;
  g_Properties.MinorNdisVersion = TAP_NDIS_MINOR_VERSION;
  g_Properties.InitializeHandler = AdapterCreate;
  g_Properties.HaltHandler = AdapterHalt;
  g_Properties.ResetHandler = AdapterReset;
  g_Properties.TransferDataHandler = AdapterReceive;
  g_Properties.SendHandler = AdapterTransmit;
  g_Properties.QueryInformationHandler = AdapterQuery;
  g_Properties.SetInformationHandler = AdapterModify;
  g_Properties.DisableInterruptHandler = NULL;
  g_Properties.EnableInterruptHandler = NULL;
  g_Properties.HandleInterruptHandler = NULL;
  g_Properties.ISRHandler = NULL;
  g_Properties.ReconfigureHandler = NULL;
  g_Properties.CheckForHangHandler = NULL;
  g_Properties.ReturnPacketHandler = NULL;
  g_Properties.SendPacketsHandler = NULL;
  g_Properties.AllocateCompleteHandler = NULL;

  g_Properties.CoCreateVcHandler = NULL;
  g_Properties.CoDeleteVcHandler = NULL;
  g_Properties.CoActivateVcHandler = NULL;
  g_Properties.CoDeactivateVcHandler = NULL;
  g_Properties.CoSendPacketsHandler = NULL;
  g_Properties.CoRequestHandler = NULL;

#ifndef ENABLE_RANDOM_MAC
  ConvertMacInfo (g_MAC, TAP_MAC_ROOT_ADDRESS, strlen (TAP_MAC_ROOT_ADDRESS));
#endif

  switch (l_Status =
	  NdisMRegisterMiniport (g_NdisWrapperHandle, &g_Properties,
				 sizeof (g_Properties)))
    {
    case NDIS_STATUS_SUCCESS:
      {
	DEBUGP (("[TAP] version [%d.%d] registered miniport successfully\n",
		  TAP_DRIVER_MAJOR_VERSION, TAP_DRIVER_MINOR_VERSION));
	break;
      }

    case NDIS_STATUS_BAD_CHARACTERISTICS:
      {
	DEBUGP (("[TAP] Miniport characteristics were badly defined\n"));
	NdisTerminateWrapper (g_NdisWrapperHandle, NULL);
	break;
      }

    case NDIS_STATUS_BAD_VERSION:
      {
	DEBUGP
	  (("[TAP] NDIS Version is wrong for the given characteristics\n"));
	NdisTerminateWrapper (g_NdisWrapperHandle, NULL);
	break;
      }

    case NDIS_STATUS_RESOURCES:
      {
	DEBUGP (("[TAP] Insufficient resources\n"));
	NdisTerminateWrapper (g_NdisWrapperHandle, NULL);
	break;
      }

    case NDIS_STATUS_FAILURE:
      {
	DEBUGP (("[TAP] Unknown fatal registration error\n"));
	NdisTerminateWrapper (g_NdisWrapperHandle, NULL);
	break;
      }
    }

  //==============================================================
  // Save original NDIS dispatch functions and override with ours
  //==============================================================
  HookDispatchFunctions ();

  return l_Status;
}

//==========================================================
//                            Adapter Initialization
//==========================================================
NDIS_STATUS AdapterCreate
  (OUT PNDIS_STATUS p_ErrorStatus,
   OUT PUINT p_MediaIndex,
   IN PNDIS_MEDIUM p_Media,
   IN UINT p_MediaCount,
   IN NDIS_HANDLE p_AdapterHandle,
   IN NDIS_HANDLE p_ConfigurationHandle)
{
  NDIS_MEDIUM l_PreferredMedium = NdisMedium802_3; // Ethernet
  TapAdapterPointer l_Adapter = NULL;
  ANSI_STRING l_AdapterString;
#ifndef ENABLE_RANDOM_MAC
  MACADDR l_MAC;
#endif
  UINT l_Index;
  NDIS_STATUS status;

  //====================================
  // Make sure adapter type is supported
  //====================================

  for (l_Index = 0;
       l_Index < p_MediaCount && p_Media[l_Index] != l_PreferredMedium;
       ++l_Index);

  if (l_Index == p_MediaCount)
    {
      DEBUGP (("[TAP] Unsupported adapter type [wanted: %d]\n",
	       l_PreferredMedium));
      return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

  *p_MediaIndex = l_Index;

  //=========================================
  // Allocate memory for TapAdapter structure
  //=========================================

  status = NdisAllocateMemoryWithTag ((PVOID *) & l_Adapter,
				      sizeof (TapAdapter), '1PAT');

  if (status != NDIS_STATUS_SUCCESS || l_Adapter == NULL)
    {
      DEBUGP (("[TAP] Couldn't allocate adapter memory\n"));
      return NDIS_STATUS_RESOURCES;
    }

  //==========================================
  // Inform the NDIS library about significant
  // features of our virtual NIC.
  //==========================================

  NdisMSetAttributesEx
    (p_AdapterHandle,
     (NDIS_HANDLE) l_Adapter,
     16,
     NDIS_ATTRIBUTE_DESERIALIZE
     | NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT
     | NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT
     | NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND,
     NdisInterfaceInternal);

  //=====================================
  // Initialize simple Adapter parameters
  //=====================================

  NdisZeroMemory (l_Adapter, sizeof (TapAdapter));

  l_Adapter->m_Lookahead = DEFAULT_PACKET_LOOKAHEAD;
  l_Adapter->m_Medium = l_PreferredMedium;
  l_Adapter->m_DeviceState = '?';

  //====================================================
  // Register a shutdown handler which will be called
  // on system restart/shutdown to halt our virtual NIC.
  //====================================================

  NdisMRegisterAdapterShutdownHandler (p_AdapterHandle, l_Adapter,
				       AdapterHalt);
  l_Adapter->m_MiniportAdapterHandle = p_AdapterHandle;
  l_Adapter->m_RegisteredAdapterShutdownHandler = TRUE;

  //===========================================================
  // Allocate spinlocks used to control access to
  // shared data structures by concurrent threads of execution.
  // QueueLock is used to lock the packet queue used
  // for the TAP-Win32 NIC -> User Space packet flow direction.
  // This code is designed to be fully SMP-safe.
  //===========================================================

  NdisAllocateSpinLock (&l_Adapter->m_Lock);
  NdisAllocateSpinLock (&l_Adapter->m_QueueLock);
  l_Adapter->m_AllocatedSpinlocks = TRUE;

  //====================================
  // Allocate and construct adapter name
  //====================================

  l_AdapterString.MaximumLength =
    ((PNDIS_MINIPORT_BLOCK) p_AdapterHandle)->MiniportName.Length + 5;

  if ((l_Adapter->m_Name = l_AdapterString.Buffer =
       ExAllocatePoolWithTag (NonPagedPool,
			      l_AdapterString.MaximumLength,
			      '2PAT')) == NULL)
    {
      AdapterFreeResources (l_Adapter);
      return NDIS_STATUS_RESOURCES;
    }

  RtlUnicodeStringToAnsiString (
             &l_AdapterString,
	     &((PNDIS_MINIPORT_BLOCK) p_AdapterHandle)->MiniportName,
	     FALSE);
  l_AdapterString.Buffer[l_AdapterString.Length] = 0;

  //==================================
  // Store and update MAC address info
  //==================================

#ifdef ENABLE_RANDOM_MAC
  GenerateRandomMac (g_MAC, l_Adapter->m_Name);
  COPY_MAC (l_Adapter->m_MAC, g_MAC);
#else
  COPY_MAC (l_Adapter->m_MAC, g_MAC);

  l_MAC[0] = g_MAC[5];
  l_MAC[1] = g_MAC[4];

  ++(*((unsigned short *) l_MAC));

  g_MAC[5] = l_MAC[0];
  g_MAC[4] = l_MAC[1];
#endif

  DEBUGP (("[%s] Using MAC %x:%x:%x:%x:%x:%x\n",
	    l_Adapter->m_Name,
	    l_Adapter->m_MAC[0], l_Adapter->m_MAC[1], l_Adapter->m_MAC[2],
	    l_Adapter->m_MAC[3], l_Adapter->m_MAC[4], l_Adapter->m_MAC[5]));

  //==================
  // Set broadcast MAC
  //==================
  {
    int i;
    for (i = 0; i < sizeof (MACADDR); ++i)
      l_Adapter->m_MAC_Broadcast[i] = 0xFF;
  }

  //============================================
  // Get parameters from registry which were set
  // in the adapter advanced properties dialog.
  //============================================
  {
    NDIS_STATUS status;
    NDIS_HANDLE configHandle;
    NDIS_CONFIGURATION_PARAMETER *parm;

    // set defaults in case our registry query fails
    l_Adapter->m_MTU = DEFAULT_PACKET_LOOKAHEAD;
    l_Adapter->m_MediaStateAlwaysConnected = FALSE;
    l_Adapter->m_MediaState = FALSE;

    NdisOpenConfiguration (&status, &configHandle, p_ConfigurationHandle);
    if (status == NDIS_STATUS_SUCCESS)
      {
	/* Read MTU setting from registry */
	{
	  NDIS_STRING key = NDIS_STRING_CONST("MTU");
	  NdisReadConfiguration (&status, &parm, configHandle,
				 &key, NdisParameterInteger);
	  if (status == NDIS_STATUS_SUCCESS)
	    {
	      if (parm->ParameterType == NdisParameterInteger)
		{
		  int mtu = parm->ParameterData.IntegerData;
		  if (mtu < MINIMUM_MTU)
		    mtu = MINIMUM_MTU;
		  if (mtu > MAXIMUM_MTU)
		    mtu = MAXIMUM_MTU;
		  l_Adapter->m_MTU = mtu;
		}
	    }
	}

	/* Read Media Status setting from registry */
	{
	  NDIS_STRING key = NDIS_STRING_CONST("MediaStatus");
	  NdisReadConfiguration (&status, &parm, configHandle,
				 &key, NdisParameterInteger);
	  if (status == NDIS_STATUS_SUCCESS)
	    {
	      if (parm->ParameterType == NdisParameterInteger)
		{
		  if (parm->ParameterData.IntegerData)
		    {
		      l_Adapter->m_MediaStateAlwaysConnected = TRUE;
		      l_Adapter->m_MediaState = TRUE;
		    }
		}
	    }
	}

	NdisCloseConfiguration (configHandle);
      }

    DEBUGP (("[%s] MTU=%d\n", l_Adapter->m_Name, l_Adapter->m_MTU));
  }

  //====================================
  // Initialize TAP device
  //====================================
  {
    NDIS_STATUS tap_status;
    tap_status = CreateTapDevice (l_Adapter);
    if (tap_status != NDIS_STATUS_SUCCESS)
      {
	AdapterFreeResources (l_Adapter);
	return tap_status;
      }
  }

  l_Adapter->m_InterfaceIsRunning = TRUE;
  return NDIS_STATUS_SUCCESS;
}

VOID
AdapterHalt (IN NDIS_HANDLE p_AdapterContext)
{
  TapAdapterPointer l_Adapter = (TapAdapterPointer) p_AdapterContext;

  DEBUGP (("[%s] is being halted\n", l_Adapter->m_Name));
  l_Adapter->m_InterfaceIsRunning = FALSE;
  NOTE_ERROR (l_Adapter);

  // Close TAP device
  if (l_Adapter->m_TapDevice)
    DestroyTapDevice (l_Adapter);

  // Free resources
  DEBUGP (("[%s] Freeing Resources\n", l_Adapter->m_Name));
  AdapterFreeResources (l_Adapter);
}

VOID
AdapterFreeResources (TapAdapterPointer p_Adapter)
{
  MYASSERT (!p_Adapter->m_CalledAdapterFreeResources);
  p_Adapter->m_CalledAdapterFreeResources = TRUE;

  if (p_Adapter->m_Name)
    {
      ExFreePool (p_Adapter->m_Name);
      p_Adapter->m_Name = NULL;
    }

  if (p_Adapter->m_AllocatedSpinlocks)
    {
      NdisFreeSpinLock (&p_Adapter->m_Lock);
      NdisFreeSpinLock (&p_Adapter->m_QueueLock);
      p_Adapter->m_AllocatedSpinlocks = FALSE;
    }

  if (p_Adapter->m_RegisteredAdapterShutdownHandler)
    {
      NdisMDeregisterAdapterShutdownHandler
        (p_Adapter->m_MiniportAdapterHandle);
      p_Adapter->m_RegisteredAdapterShutdownHandler = FALSE;
    }

  NdisZeroMemory ((PVOID) p_Adapter, sizeof (TapAdapter));
  NdisFreeMemory ((PVOID) p_Adapter, sizeof (TapAdapter), 0);
}

//========================================================================
//                             Tap Device Initialization
//========================================================================
NDIS_STATUS
CreateTapDevice (TapAdapterPointer p_Adapter)
{
  unsigned short l_AdapterLength =
    (unsigned short) strlen (p_Adapter->m_Name);
  ANSI_STRING l_TapString, l_LinkString;
  TapExtensionPointer l_Extension;
  UNICODE_STRING l_TapUnicode;
  BOOLEAN l_FreeTapUnicode = FALSE;
  NTSTATUS l_Status, l_Return = NDIS_STATUS_SUCCESS;

  l_LinkString.Buffer = NULL;

  l_TapString.MaximumLength = l_LinkString.MaximumLength =
    l_AdapterLength + strlen (TAPSUFFIX);

  DEBUGP (("[TAP] version [%d.%d] creating tap device: %s\n",
	   TAP_DRIVER_MAJOR_VERSION,
	   TAP_DRIVER_MINOR_VERSION,
	   p_Adapter->m_Name));

  //==================================
  // Allocate pool for TAP device name
  //==================================

  if ((p_Adapter->m_TapName = l_TapString.Buffer =
       ExAllocatePoolWithTag (NonPagedPool,
			      l_TapString.MaximumLength,
			      '3PAT')) == NULL)
    {
      DEBUGP (("[%s] couldn't alloc TAP name buffer\n", p_Adapter->m_Name));
      l_Return = NDIS_STATUS_RESOURCES;
      goto cleanup;
    }

  //================================================
  // Allocate pool for TAP symbolic link name buffer
  //================================================

  if ((l_LinkString.Buffer =
       ExAllocatePoolWithTag (NonPagedPool,
			      l_LinkString.MaximumLength,
			      '4PAT')) == NULL)
    {
      DEBUGP (("[%s] couldn't alloc TAP symbolic link name buffer\n",
	       p_Adapter->m_Name));
      l_Return = NDIS_STATUS_RESOURCES;
      goto cleanup;
    }

  //=======================================================
  // Modify for tap device name ("\Device\TAPn.tap")
  //=======================================================
  NdisMoveMemory (l_TapString.Buffer, p_Adapter->m_Name, l_AdapterLength);
  NdisMoveMemory (l_TapString.Buffer + l_AdapterLength, TAPSUFFIX,
		  strlen (TAPSUFFIX) + 1);
  NdisMoveMemory (l_TapString.Buffer, "\\Device", 7);	// For Win2K problem
  l_TapString.Length = l_AdapterLength + strlen (TAPSUFFIX);

  //=======================================================
  // And modify for tap link name ("\??\TAPn.tap")
  //=======================================================
  NdisMoveMemory (l_LinkString.Buffer, l_TapString.Buffer,
		  l_TapString.Length);
  NdisMoveMemory (l_LinkString.Buffer, USERDEVICEDIR, strlen (USERDEVICEDIR));

  NdisMoveMemory
    (l_LinkString.Buffer + strlen (USERDEVICEDIR),
     l_LinkString.Buffer + strlen (SYSDEVICEDIR),
     l_TapString.Length - strlen (SYSDEVICEDIR));

  l_LinkString.Buffer[l_LinkString.Length =
		      l_TapString.Length - (strlen (SYSDEVICEDIR) -
					    strlen (USERDEVICEDIR))] = 0;

  //==================================================
  // Create new tap device and associate with adapter
  //==================================================
  if (RtlAnsiStringToUnicodeString (&l_TapUnicode, &l_TapString, TRUE) !=
      STATUS_SUCCESS)
    {
      DEBUGP (("[%s] couldn't alloc TAP unicode name buffer\n",
		p_Adapter->m_Name));
      l_Return = NDIS_STATUS_RESOURCES;
      goto cleanup;
    }
  l_FreeTapUnicode = TRUE;

  l_Status = IoCreateDevice
    (g_TapDriverObject,
     sizeof (TapExtension),
     &l_TapUnicode,
     FILE_DEVICE_PHYSICAL_NETCARD | 0x8000,
     0, FALSE, &(p_Adapter->m_TapDevice));

  if (l_Status != STATUS_SUCCESS)
    {
      DEBUGP (("[%s] couldn't be created\n", p_Adapter->m_TapName));
      l_Return = NDIS_STATUS_RESOURCES;
      goto cleanup;
    }

  if (RtlAnsiStringToUnicodeString
      (&p_Adapter->m_UnicodeLinkName, &l_LinkString, TRUE)
      != STATUS_SUCCESS)
    {
      DEBUGP
	(("[%s] Couldn't allocate unicode string for symbolic link name\n",
	 p_Adapter->m_Name));
      l_Return = NDIS_STATUS_RESOURCES;
      goto cleanup;
    }
  p_Adapter->m_CreatedUnicodeLinkName = TRUE;

  //==================================================
  // Associate symbolic link with new device
  //==================================================
  if (!NT_SUCCESS
      (IoCreateSymbolicLink (&p_Adapter->m_UnicodeLinkName, &l_TapUnicode)))
    {
      DEBUGP (("[%s] symbolic link couldn't be created\n",
		l_LinkString.Buffer));
      l_Return = NDIS_STATUS_RESOURCES;
      goto cleanup;
    }
  p_Adapter->m_CreatedSymbolLink = TRUE;

  //==================================================
  // Initialize device extension (basically a kind
  // of private data area containing our state info).
  //==================================================

  l_Extension =
    ((TapExtensionPointer) p_Adapter->m_TapDevice->DeviceExtension);

  NdisZeroMemory (l_Extension, sizeof (TapExtension));

  p_Adapter->m_DeviceExtensionIsAccessible = TRUE;

  l_Extension->m_Adapter = p_Adapter;
  l_Extension->m_halt = FALSE;

  //========================================================
  // Initialize Packet and IRP queues.
  //
  // The packet queue is used to buffer data which has been
  // "transmitted" by the virtual NIC, before user space
  // has had a chance to read it.
  //
  // The IRP queue is used to buffer pending I/O requests
  // from userspace, i.e. read requests on the TAP device
  // waiting for the system to "transmit" something through
  // the virtual NIC.
  //
  // Basically, packets in the packet queue are used
  // to satisfy IRP requests in the IRP queue.
  //
  // All accesses to packet or IRP queues should be
  // bracketed by the QueueLock spinlock,
  // in order to be SMP-safe.
  //========================================================

  l_Extension->m_PacketQueue = QueueInit (PACKET_QUEUE_SIZE);
  l_Extension->m_IrpQueue = QueueInit (IRP_QUEUE_SIZE);

  if (!l_Extension->m_PacketQueue
      || !l_Extension->m_IrpQueue)
    {
      DEBUGP (("[%s] couldn't alloc TAP queues\n", p_Adapter->m_Name));
      l_Return = NDIS_STATUS_RESOURCES;
      goto cleanup;
    }

  //========================
  // Finalize initialization
  //========================

  p_Adapter->m_TapIsRunning = TRUE;

  /* instead of DO_BUFFERED_IO */
  p_Adapter->m_TapDevice->Flags |= DO_DIRECT_IO;

  p_Adapter->m_TapDevice->Flags &= ~DO_DEVICE_INITIALIZING;

  DEBUGP (("[%s] successfully created TAP device [%s]\n", p_Adapter->m_Name,
	    p_Adapter->m_TapName));

 cleanup:
  if (l_FreeTapUnicode)
    RtlFreeUnicodeString (&l_TapUnicode);
  if (l_LinkString.Buffer)
    ExFreePool (l_LinkString.Buffer);

  if (l_Return != NDIS_STATUS_SUCCESS)
    TapDeviceFreeResources (p_Adapter);

  return l_Return;
}

VOID
DestroyTapDevice (TapAdapterPointer p_Adapter)
{
  TapExtensionPointer l_Extension =
    (TapExtensionPointer) p_Adapter->m_TapDevice->DeviceExtension;

  DEBUGP (("[%s] Destroying tap device\n", p_Adapter->m_TapName));

  //======================================
  // Let clients know we are shutting down
  //======================================
  p_Adapter->m_TapIsRunning = FALSE;
  p_Adapter->m_TapOpens = 0;
  l_Extension->m_halt = TRUE;

  //====================================
  // Give clients time to finish up.
  // Note that we must be running at IRQL
  // < DISPATCH_LEVEL in order to call
  // NdisMSleep.
  //====================================
  NdisMSleep (1000);        

  //================================
  // Exhaust IRP and packet queues
  //================================
  FlushQueues (p_Adapter);

  TapDeviceFreeResources (p_Adapter);
}

VOID
TapDeviceFreeResources (TapAdapterPointer p_Adapter)
{
  MYASSERT (p_Adapter);
  MYASSERT (!p_Adapter->m_CalledTapDeviceFreeResources);
  p_Adapter->m_CalledTapDeviceFreeResources = TRUE;

  if (p_Adapter->m_DeviceExtensionIsAccessible)
    {
      TapExtensionPointer l_Extension;

      MYASSERT (p_Adapter->m_TapDevice);
      l_Extension = (TapExtensionPointer)
	p_Adapter->m_TapDevice->DeviceExtension;
      MYASSERT (l_Extension);

      if (l_Extension->m_PacketQueue)
	QueueFree (l_Extension->m_PacketQueue);
      if (l_Extension->m_IrpQueue)
	QueueFree (l_Extension->m_IrpQueue);

      p_Adapter->m_DeviceExtensionIsAccessible = FALSE;
    }

  if (p_Adapter->m_CreatedSymbolLink)
    IoDeleteSymbolicLink (&p_Adapter->m_UnicodeLinkName);

  if (p_Adapter->m_CreatedUnicodeLinkName)
    RtlFreeUnicodeString (&p_Adapter->m_UnicodeLinkName);

  //==========================================================
  // According to DDK docs, the device is not actually deleted
  // until its reference count falls to zero.  That means we
  // still need to gracefully fail TapDeviceHook requests
  // after this point, otherwise ugly things would happen if
  // the device was disabled (e.g. in the network connections
  // control panel) while a userspace app still held an open
  // file handle to it.
  //==========================================================
  
  if (p_Adapter->m_TapDevice)
    {
      IoDeleteDevice (p_Adapter->m_TapDevice);
      p_Adapter->m_TapDevice = NULL;
    }

  if (p_Adapter->m_TapName)
    {
      ExFreePool (p_Adapter->m_TapName);
      p_Adapter->m_TapName = NULL;
    }
}

//========================================================
//                      Adapter Control
//========================================================
NDIS_STATUS
AdapterReset (OUT PBOOLEAN p_AddressingReset, IN NDIS_HANDLE p_AdapterContext)
{
  TapAdapterPointer l_Adapter = (TapAdapterPointer) p_AdapterContext;
  DEBUGP (("[%s] is resetting\n", l_Adapter->m_Name));
  return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS AdapterReceive
  (OUT PNDIS_PACKET p_Packet,
   OUT PUINT p_Transferred,
   IN NDIS_HANDLE p_AdapterContext,
   IN NDIS_HANDLE p_ReceiveContext, IN UINT p_Offset, IN UINT p_ToTransfer)
{
  return NDIS_STATUS_SUCCESS;
}

//==============================================================
//                  Adapter Option Query/Modification
//==============================================================
NDIS_STATUS AdapterQuery
  (IN NDIS_HANDLE p_AdapterContext,
   IN NDIS_OID p_OID,
   IN PVOID p_Buffer,
   IN ULONG p_BufferLength,
   OUT PULONG p_BytesWritten, OUT PULONG p_BytesNeeded)
{
  TapAdapterPointer l_Adapter = (TapAdapterPointer) p_AdapterContext;
  TapAdapterQuery l_Query, *l_QueryPtr = &l_Query;
  NDIS_STATUS l_Status = NDIS_STATUS_SUCCESS;
  UINT l_QueryLength = 4;

  NdisZeroMemory (&l_Query, sizeof (l_Query));
  NdisAcquireSpinLock (&l_Adapter->m_Lock);

  switch (p_OID)
    {
      //===================================================================
      //                       Vendor & Driver version Info
      //===================================================================
    case OID_GEN_VENDOR_DESCRIPTION:
      l_QueryPtr = (TapAdapterQueryPointer) PRODUCT_STRING;
      l_QueryLength = strlen (PRODUCT_STRING) + 1;
      break;

    case OID_GEN_VENDOR_ID:
      l_Query.m_Long = 0xffffff;
      break;

    case OID_GEN_DRIVER_VERSION:
      l_Query.m_Short =
	(((USHORT) TAP_NDIS_MAJOR_VERSION) << 8 | (USHORT)
	 TAP_NDIS_MINOR_VERSION);
      l_QueryLength = sizeof (unsigned short);
      break;

    case OID_GEN_VENDOR_DRIVER_VERSION:
      l_Query.m_Long =
	(((USHORT) TAP_DRIVER_MAJOR_VERSION) << 8 | (USHORT)
	 TAP_DRIVER_MINOR_VERSION);
      break;

      //=================================================================
      //                             Statistics
      //=================================================================
    case OID_GEN_RCV_NO_BUFFER:
      l_Query.m_Long = 0;
      break;

    case OID_802_3_RCV_ERROR_ALIGNMENT:
      l_Query.m_Long = 0;
      break;

    case OID_802_3_XMIT_ONE_COLLISION:
      l_Query.m_Long = 0;
      break;

    case OID_802_3_XMIT_MORE_COLLISIONS:
      l_Query.m_Long = 0;
      break;

    case OID_GEN_XMIT_OK:
      l_Query.m_Long = l_Adapter->m_Tx;
      break;

    case OID_GEN_RCV_OK:
      l_Query.m_Long = l_Adapter->m_Rx;
      break;

    case OID_GEN_XMIT_ERROR:
      l_Query.m_Long = l_Adapter->m_TxErr;
      break;

    case OID_GEN_RCV_ERROR:
      l_Query.m_Long = l_Adapter->m_RxErr;
      break;

      //===================================================================
      //                       Device & Protocol Options
      //===================================================================
    case OID_GEN_SUPPORTED_LIST:
      l_QueryPtr = (TapAdapterQueryPointer) g_SupportedOIDList;
      l_QueryLength = sizeof (g_SupportedOIDList);
      break;

    case OID_GEN_MAC_OPTIONS:
      // This MUST be here !!!
      l_Query.m_Long = (NDIS_MAC_OPTION_RECEIVE_SERIALIZED
			| NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA
			| NDIS_MAC_OPTION_NO_LOOPBACK
			| NDIS_MAC_OPTION_TRANSFERS_NOT_PEND);

      break;

    case OID_GEN_CURRENT_PACKET_FILTER:
      l_Query.m_Long =
	(NDIS_PACKET_TYPE_ALL_LOCAL |
	 NDIS_PACKET_TYPE_BROADCAST |
	 NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_ALL_FUNCTIONAL);

      break;

    case OID_GEN_PROTOCOL_OPTIONS:
      l_Query.m_Long = 0;
      break;

      //==================================================================
      //                            Device Info
      //==================================================================
    case OID_GEN_MEDIA_CONNECT_STATUS:
      l_Query.m_Long = l_Adapter->m_MediaState
	? NdisMediaStateConnected : NdisMediaStateDisconnected;
      break;

    case OID_GEN_HARDWARE_STATUS:
      l_Query.m_HardwareStatus = NdisHardwareStatusReady;
      l_QueryLength = sizeof (NDIS_HARDWARE_STATUS);
      break;

    case OID_GEN_MEDIA_SUPPORTED:
    case OID_GEN_MEDIA_IN_USE:
      l_Query.m_Medium = l_Adapter->m_Medium;
      l_QueryLength = sizeof (NDIS_MEDIUM);
      break;

    case OID_GEN_LINK_SPEED:
      l_Query.m_Long = 100000;
      break;

    case OID_802_3_MULTICAST_LIST:
      l_Query.m_Long = 0;
      break;

    case OID_802_3_PERMANENT_ADDRESS:
    case OID_802_3_CURRENT_ADDRESS:
      COPY_MAC (l_Query.m_MacAddress, l_Adapter->m_MAC);
      l_QueryLength = sizeof (MACADDR);
      break;

      //==================================================================
      //                             Limits
      //==================================================================

    case OID_GEN_MAXIMUM_SEND_PACKETS:
      l_Query.m_Long = 1;
      break;

    case OID_802_3_MAXIMUM_LIST_SIZE:
      l_Query.m_Long = 0;
      break;

    case OID_GEN_CURRENT_LOOKAHEAD:
      l_Query.m_Long = l_Adapter->m_Lookahead;
      break;

    case OID_GEN_MAXIMUM_LOOKAHEAD:
    case OID_GEN_MAXIMUM_TOTAL_SIZE:
    case OID_GEN_RECEIVE_BUFFER_SPACE:
    case OID_GEN_RECEIVE_BLOCK_SIZE:
      l_Query.m_Long = DEFAULT_PACKET_LOOKAHEAD;
      break;

    case OID_GEN_MAXIMUM_FRAME_SIZE:
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
    case OID_GEN_TRANSMIT_BUFFER_SPACE:
      l_Query.m_Long = l_Adapter->m_MTU;
      break;

    case OID_PNP_CAPABILITIES:
      do
	{
	  PNDIS_PNP_CAPABILITIES pPNPCapabilities;
	  PNDIS_PM_WAKE_UP_CAPABILITIES pPMstruct;

	  if (p_BufferLength >= sizeof (NDIS_PNP_CAPABILITIES))
	    {
	      pPNPCapabilities = (PNDIS_PNP_CAPABILITIES) (p_Buffer);

	      //
	      // Setting up the buffer to be returned
	      // to the Protocol above the Passthru miniport
	      //
	      pPMstruct = &pPNPCapabilities->WakeUpCapabilities;
	      pPMstruct->MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
	      pPMstruct->MinPatternWakeUp = NdisDeviceStateUnspecified;
	      pPMstruct->MinLinkChangeWakeUp = NdisDeviceStateUnspecified;
	    }
	  l_QueryLength = sizeof (NDIS_PNP_CAPABILITIES);
	}
      while (FALSE);
      break;
    case OID_PNP_QUERY_POWER:
      break;

      // Required OIDs that we don't support

    case OID_GEN_SUPPORTED_GUIDS:
    case OID_GEN_MEDIA_CAPABILITIES:
    case OID_GEN_PHYSICAL_MEDIUM:
    case OID_TCP_TASK_OFFLOAD:
    case OID_FFP_SUPPORT:
      l_Status = NDIS_STATUS_INVALID_OID;
      break;

      // Optional stats OIDs

    case OID_GEN_DIRECTED_BYTES_XMIT:
    case OID_GEN_DIRECTED_FRAMES_XMIT:
    case OID_GEN_MULTICAST_BYTES_XMIT:
    case OID_GEN_MULTICAST_FRAMES_XMIT:
    case OID_GEN_BROADCAST_BYTES_XMIT:
    case OID_GEN_BROADCAST_FRAMES_XMIT:
    case OID_GEN_DIRECTED_BYTES_RCV:
    case OID_GEN_DIRECTED_FRAMES_RCV:
    case OID_GEN_MULTICAST_BYTES_RCV:
    case OID_GEN_MULTICAST_FRAMES_RCV:
    case OID_GEN_BROADCAST_BYTES_RCV:
    case OID_GEN_BROADCAST_FRAMES_RCV:
      l_Status = NDIS_STATUS_INVALID_OID;
      break;

      //===================================================================
      //                          Not Handled
      //===================================================================
    default:
      DEBUGP (("[%s] Unhandled OID %lx\n", l_Adapter->m_Name, p_OID));
      l_Status = NDIS_STATUS_INVALID_OID;
      break;
    }

  if (l_Status != NDIS_STATUS_SUCCESS)
    ;
  else if (l_QueryLength > p_BufferLength)
    {
      l_Status = NDIS_STATUS_INVALID_LENGTH;
      *p_BytesNeeded = l_QueryLength;
    }
  else
    NdisMoveMemory (p_Buffer, (PVOID) l_QueryPtr,
		    (*p_BytesWritten = l_QueryLength));

  NdisReleaseSpinLock (&l_Adapter->m_Lock);

  return l_Status;
}

NDIS_STATUS AdapterModify
  (IN NDIS_HANDLE p_AdapterContext,
   IN NDIS_OID p_OID,
   IN PVOID p_Buffer,
   IN ULONG p_BufferLength,
   OUT PULONG p_BytesRead,
   OUT PULONG p_BytesNeeded)
{
  TapAdapterQueryPointer l_Query = (TapAdapterQueryPointer) p_Buffer;
  TapAdapterPointer l_Adapter = (TapAdapterPointer) p_AdapterContext;
  NDIS_STATUS l_Status = NDIS_STATUS_INVALID_OID;
  ULONG l_Long;

  NdisAcquireSpinLock (&l_Adapter->m_Lock);

  switch (p_OID)
    {
      //==================================================================
      //                            Device Info
      //==================================================================
    case OID_802_3_MULTICAST_LIST:
      DEBUGP (("[%s] Setting [OID_802_3_MAXIMUM_LIST_SIZE]\n",
		l_Adapter->m_Name));
      l_Status = NDIS_STATUS_SUCCESS;
      break;

    case OID_GEN_CURRENT_PACKET_FILTER:
      l_Status = NDIS_STATUS_INVALID_LENGTH;
      *p_BytesNeeded = 4;

      if (p_BufferLength >= sizeof (ULONG))
	{
	  DEBUGP
	    (("[%s] Setting [OID_GEN_CURRENT_PACKET_FILTER] to [0x%02lx]\n",
	     l_Adapter->m_Name, l_Query->m_Long));
	  l_Status = NDIS_STATUS_SUCCESS;
	  *p_BytesRead = sizeof (ULONG);
	}

      break;

    case OID_GEN_CURRENT_LOOKAHEAD:
      if (p_BufferLength < sizeof (ULONG))
	{
	  l_Status = NDIS_STATUS_INVALID_LENGTH;
	  *p_BytesNeeded = 4;
	}
      else if (l_Query->m_Long > DEFAULT_PACKET_LOOKAHEAD
	       || l_Query->m_Long <= 0)
	l_Status = NDIS_STATUS_INVALID_DATA;
      else
	{
	  DEBUGP (("[%s] Setting [OID_GEN_CURRENT_LOOKAHEAD] to [%d]\n",
		    l_Adapter->m_Name, l_Query->m_Long));
	  l_Adapter->m_Lookahead = l_Query->m_Long;
	  l_Status = NDIS_STATUS_SUCCESS;
	  *p_BytesRead = sizeof (ULONG);
	}

      break;

    case OID_GEN_NETWORK_LAYER_ADDRESSES:
      l_Status = NDIS_STATUS_SUCCESS;
      *p_BytesRead = *p_BytesNeeded = 0;
      break;

    case OID_GEN_TRANSPORT_HEADER_OFFSET:
      l_Status = NDIS_STATUS_SUCCESS;
      *p_BytesRead = *p_BytesNeeded = 0;
      break;

    case OID_PNP_SET_POWER:
      do
	{
	  NDIS_DEVICE_POWER_STATE NewDeviceState;

	  NewDeviceState = (*(PNDIS_DEVICE_POWER_STATE) p_Buffer);

	  switch (NewDeviceState)
	    {
	    case NdisDeviceStateD0:
	      l_Adapter->m_DeviceState = '0';
	      break;
	    case NdisDeviceStateD1:
	      l_Adapter->m_DeviceState = '1';
	      break;
	    case NdisDeviceStateD2:
	      l_Adapter->m_DeviceState = '2';
	      break;
	    case NdisDeviceStateD3:
	      l_Adapter->m_DeviceState = '3';
	      break;
	    default:
	      l_Adapter->m_DeviceState = '?';
	      break;
	    }

	  l_Status = NDIS_STATUS_FAILURE;

	  //
	  // Check for invalid length
	  //
	  if (p_BufferLength < sizeof (NDIS_DEVICE_POWER_STATE))
	    {
	      l_Status = NDIS_STATUS_INVALID_LENGTH;
	      break;
	    }

	  if (NewDeviceState > NdisDeviceStateD0)
	    {
	      l_Adapter->m_InterfaceIsRunning = FALSE;
	      DEBUGP (("[%s] Power management device state OFF\n",
		       l_Adapter->m_Name));
	    }
	  else
	    {
	      l_Adapter->m_InterfaceIsRunning = TRUE;
	      DEBUGP (("[%s] Power management device state ON\n",
		       l_Adapter->m_Name));
	    }

	  l_Status = NDIS_STATUS_SUCCESS;
	}
      while (FALSE);

      if (l_Status == NDIS_STATUS_SUCCESS)
	{
	  *p_BytesRead = sizeof (NDIS_DEVICE_POWER_STATE);
	  *p_BytesNeeded = 0;
	}
      else
	{
	  *p_BytesRead = 0;
	  *p_BytesNeeded = sizeof (NDIS_DEVICE_POWER_STATE);
	}
      break;

    case OID_PNP_REMOVE_WAKE_UP_PATTERN:
    case OID_PNP_ADD_WAKE_UP_PATTERN:
      l_Status = NDIS_STATUS_SUCCESS;
      *p_BytesRead = *p_BytesNeeded = 0;
      break;

    default:
      DEBUGP (("[%s] Can't set value for OID %lx\n", l_Adapter->m_Name,
		p_OID));
      l_Status = NDIS_STATUS_INVALID_OID;
      *p_BytesRead = *p_BytesNeeded = 0;
      break;
    }

  NdisReleaseSpinLock (&l_Adapter->m_Lock);

  return l_Status;
}

//====================================================================
//                               Adapter Transmission
//====================================================================
NDIS_STATUS
AdapterTransmit (IN NDIS_HANDLE p_AdapterContext, IN PNDIS_PACKET p_Packet,
		 IN UINT p_Flags)
{
  TapAdapterPointer l_Adapter = (TapAdapterPointer) p_AdapterContext;
  ULONG l_Index = 0, l_BufferLength = 0, l_PacketLength = 0;
  PIRP l_IRP;
  TapPacketPointer l_PacketBuffer;
  TapExtensionPointer l_Extension;
  PNDIS_BUFFER l_NDIS_Buffer;
  PUCHAR l_Buffer;
  PVOID result;

  NdisQueryPacket (p_Packet, NULL, NULL, &l_NDIS_Buffer, &l_PacketLength);

  //====================================================
  // Here we abandon the transmission attempt if any of
  // the parameters is wrong or memory allocation fails
  // but we do not indicate failure. The packet is
  // silently dropped.
  //====================================================

  if (l_Adapter->m_TapDevice == NULL)
    return NDIS_STATUS_FAILURE;
  else if ((l_Extension =
	    (TapExtensionPointer) l_Adapter->m_TapDevice->DeviceExtension) ==
	   NULL)
    return NDIS_STATUS_FAILURE;
  else if (l_PacketLength < ETHERNET_HEADER_SIZE)
    return NDIS_STATUS_FAILURE;
  else if (l_PacketLength > 65535)  // Cap packet size to TCP/IP maximum
    return NDIS_STATUS_FAILURE;
  else if (!l_Adapter->m_TapOpens || !l_Adapter->m_MediaState)
    return NDIS_STATUS_SUCCESS;     // Nothing is bound to the TAP device

  if (NdisAllocateMemoryWithTag (&l_PacketBuffer,
				 TAP_PACKET_SIZE (l_PacketLength),
				 '5PAT') != NDIS_STATUS_SUCCESS)
    return NDIS_STATUS_RESOURCES;

  if (l_PacketBuffer == NULL)
    return NDIS_STATUS_RESOURCES;

  l_PacketBuffer->m_SizeFlags = (l_PacketLength & TP_SIZE_MASK);

  //===========================
  // Reassemble packet contents
  //===========================

  __try
  {
    for (l_Index = 0; l_NDIS_Buffer && l_Index < l_PacketLength;
	 l_Index += l_BufferLength)
      {
	NdisQueryBuffer (l_NDIS_Buffer, (PVOID *) & l_Buffer,
			 &l_BufferLength);
	NdisMoveMemory (l_PacketBuffer->m_Data + l_Index, l_Buffer,
			l_BufferLength);
	NdisGetNextBuffer (l_NDIS_Buffer, &l_NDIS_Buffer);
      }

    DEBUG_PACKET ("AdapterTransmit", l_PacketBuffer->m_Data, l_PacketLength);

    //===============================================
    // In Point-To-Point mode, check to see whether
    // packet is ARP or IPv4 (if neither, then drop).
    //===============================================
    if (l_Adapter->m_PointToPoint)
      {
	ETH_HEADER *e;

	if (l_PacketLength < ETHERNET_HEADER_SIZE)
	  goto no_queue;

	e = (ETH_HEADER *) l_PacketBuffer->m_Data;

	switch (ntohs (e->proto))
	  {
	  case ETH_P_ARP:

	    // Make sure that packet is the
	    // right size for ARP.
	    if (l_PacketLength != sizeof (ARP_PACKET))
	      goto no_queue;

	    ProcessARP (l_Adapter, (PARP_PACKET) l_PacketBuffer->m_Data);

	  default:
	    goto no_queue;

	  case ETH_P_IP:

	    // Make sure that packet is large
	    // enough to be IPv4.
	    if (l_PacketLength
		< ETHERNET_HEADER_SIZE + IP_HEADER_SIZE)
	      goto no_queue;

	    // Only accept directed packets,
	    // not broadcasts.
	    if (memcmp (e, &l_Adapter->m_TapToUser, ETHERNET_HEADER_SIZE))
	      goto no_queue;

	    // Packet looks like IPv4, queue it.
	    l_PacketBuffer->m_SizeFlags |= TP_POINT_TO_POINT;
	  }
      }

    //===============================================
    // Push packet onto queue to wait for read from
    // userspace.
    //===============================================

    NdisAcquireSpinLock (&l_Adapter->m_QueueLock);

    result = NULL;
    if (IS_UP (l_Adapter))
      result = QueuePush (l_Extension->m_PacketQueue, l_PacketBuffer);

    NdisReleaseSpinLock (&l_Adapter->m_QueueLock);

    if ((TapPacketPointer) result != l_PacketBuffer)
      {
	// adapter receive overrun
#ifndef NEED_TAP_IOCTL_SET_STATISTICS
	INCREMENT_STAT (l_Adapter->m_TxErr);
#endif
	goto no_queue;
      }
#ifndef NEED_TAP_IOCTL_SET_STATISTICS
    else
      {
	INCREMENT_STAT (l_Adapter->m_Tx);
      }
#endif

    //============================================================
    // Cycle through IRPs and packets, try to satisfy each pending
    // IRP with a queued packet.
    //============================================================
    while (TRUE)
      {
	l_IRP = NULL;
	l_PacketBuffer = NULL;

	NdisAcquireSpinLock (&l_Adapter->m_QueueLock);

	if (IS_UP (l_Adapter)
	    && QueueCount (l_Extension->m_PacketQueue)
	    && QueueCount (l_Extension->m_IrpQueue))
	  {
	    l_IRP = (PIRP) QueuePop (l_Extension->m_IrpQueue);
	    l_PacketBuffer = (TapPacketPointer)
	      QueuePop (l_Extension->m_PacketQueue);
	  }

	NdisReleaseSpinLock (&l_Adapter->m_QueueLock);

	MYASSERT ((l_IRP != NULL) + (l_PacketBuffer != NULL) != 1);

	if (l_IRP && l_PacketBuffer)
	  {
	    CompleteIRP (l_Adapter,
			 l_IRP,
			 l_PacketBuffer, 
			 IO_NETWORK_INCREMENT);
	  }
	else
	  break;
      }
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

  return NDIS_STATUS_SUCCESS;

 no_queue:
  NdisFreeMemory (l_PacketBuffer,
		  TAP_PACKET_SIZE (l_PacketLength),
		  0);
  return NDIS_STATUS_SUCCESS;
}

//======================================================================
// Hook for catching tap device IRP's.
// Network adapter requests are forwarded on to NDIS
//======================================================================
NTSTATUS
TapDeviceHook (IN PDEVICE_OBJECT p_DeviceObject, IN PIRP p_IRP)
{
  PIO_STACK_LOCATION l_IrpSp;
  TapExtensionPointer l_Extension;
  NTSTATUS l_Status = STATUS_SUCCESS;
  TapAdapterPointer l_Adapter;

  //=======================================================================
  //  If it's not the private data device type, call the original handler
  //=======================================================================
  l_IrpSp = IoGetCurrentIrpStackLocation (p_IRP);
  if (p_DeviceObject->DeviceType != (FILE_DEVICE_PHYSICAL_NETCARD | 0x8000))
    {
      return (*g_DispatchHook[l_IrpSp->MajorFunction]) (p_DeviceObject,
							p_IRP);
    }

  //=============================================================
  //     Only TAP device I/O requests get handled below here
  //=============================================================
  l_Extension = (TapExtensionPointer) p_DeviceObject->DeviceExtension;
  l_Adapter = l_Extension->m_Adapter;

  p_IRP->IoStatus.Status = STATUS_SUCCESS;
  p_IRP->IoStatus.Information = 0;

  if (l_Extension->m_halt)
    {
      DEBUGP (("TapDeviceHook called when TAP device is halted\n"));
      p_IRP->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
      IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
      return STATUS_NO_SUCH_DEVICE;
    }

  switch (l_IrpSp->MajorFunction)
    {
      //===========================================================
      //                 Ioctl call handlers
      //===========================================================
    case IRP_MJ_DEVICE_CONTROL:
      {
	switch (l_IrpSp->Parameters.DeviceIoControl.IoControlCode)
	  {
	  case TAP_IOCTL_GET_MAC:
	    {
	      if (l_IrpSp->Parameters.DeviceIoControl.OutputBufferLength
		  >= sizeof (MACADDR))
		{
		  COPY_MAC (p_IRP->AssociatedIrp.SystemBuffer,
			    l_Adapter->m_MAC);
		  p_IRP->IoStatus.Information = sizeof (MACADDR);
		}
	      else
		{
		  NOTE_ERROR (l_Adapter);
		  p_IRP->IoStatus.Status = l_Status = STATUS_BUFFER_TOO_SMALL;
		}
	      break;
	    }
	  case TAP_IOCTL_GET_VERSION:
	    {
	      const ULONG size = sizeof (ULONG) * 3;
	      if (l_IrpSp->Parameters.DeviceIoControl.OutputBufferLength
		  >= size)
		{
		  ((PULONG) (p_IRP->AssociatedIrp.SystemBuffer))[0]
		    = TAP_DRIVER_MAJOR_VERSION;
		  ((PULONG) (p_IRP->AssociatedIrp.SystemBuffer))[1]
		    = TAP_DRIVER_MINOR_VERSION;
		  ((PULONG) (p_IRP->AssociatedIrp.SystemBuffer))[2]
#if DBG
		    = 1;
#else
		    = 0;
#endif
		  p_IRP->IoStatus.Information = size;
		}
	      else
		{
		  NOTE_ERROR (l_Adapter);
		  p_IRP->IoStatus.Status = l_Status = STATUS_BUFFER_TOO_SMALL;
		}

	      break;
	    }
	  case TAP_IOCTL_GET_MTU:
	    {
	      const ULONG size = sizeof (ULONG) * 1;
	      if (l_IrpSp->Parameters.DeviceIoControl.OutputBufferLength
		  >= size)
		{
		  ((PULONG) (p_IRP->AssociatedIrp.SystemBuffer))[0]
		    = l_Adapter->m_MTU;
		  p_IRP->IoStatus.Information = size;
		}
	      else
		{
		  NOTE_ERROR (l_Adapter);
		  p_IRP->IoStatus.Status = l_Status = STATUS_BUFFER_TOO_SMALL;
		}

	      break;
	    }
	  case TAP_IOCTL_GET_INFO:
	    {
	      char state[16];
	      if (l_Adapter->m_InterfaceIsRunning)
		state[0] = 'A';
	      else
		state[0] = 'a';
	      if (l_Adapter->m_TapIsRunning)
		state[1] = 'T';
	      else
		state[1] = 't';
	      state[2] = l_Adapter->m_DeviceState;
	      if (l_Adapter->m_MediaStateAlwaysConnected)
		state[3] = 'C';
	      else
		state[3] = 'c';
	      state[4] = '\0';

	      p_IRP->IoStatus.Status = l_Status = RtlStringCchPrintfExA (
	        ((LPTSTR) (p_IRP->AssociatedIrp.SystemBuffer)),
		l_IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
		NULL,
		NULL,
		STRSAFE_FILL_BEHIND_NULL | STRSAFE_IGNORE_NULLS,
		"State=%s Err=[%s/%d] #O=%d Tx=[%d,%d] Rx=[%d,%d] IrpQ=[%d,%d,%d] PktQ=[%d,%d,%d]",
		state,
		l_Adapter->m_LastErrorFilename,
		l_Adapter->m_LastErrorLineNumber,
		(int)l_Adapter->m_NumTapOpens,
		(int)l_Adapter->m_Tx,
		(int)l_Adapter->m_TxErr,
		(int)l_Adapter->m_Rx,
		(int)l_Adapter->m_RxErr,
		(int)l_Extension->m_IrpQueue->size,
		(int)l_Extension->m_IrpQueue->max_size,
		(int)IRP_QUEUE_SIZE,
		(int)l_Extension->m_PacketQueue->size,
		(int)l_Extension->m_PacketQueue->max_size,
		(int)PACKET_QUEUE_SIZE
		);

	      p_IRP->IoStatus.Information
		= l_IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

	      break;
	    }

#ifdef NEED_TAP_IOCTL_GET_LASTMAC
	  case TAP_IOCTL_GET_LASTMAC:
	    {
	      if (l_IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
		  sizeof (MACADDR))
		{
		  COPY_MAC (p_IRP->AssociatedIrp.SystemBuffer, g_MAC);
		  p_IRP->IoStatus.Information = sizeof (MACADDR);
		}
	      else
		{
		  NOTE_ERROR (l_Adapter);
		  p_IRP->IoStatus.Status = l_Status = STATUS_BUFFER_TOO_SMALL;
		}

	      break;
	    }
#endif

#ifdef NEED_TAP_IOCTL_SET_STATISTICS
	  case TAP_IOCTL_SET_STATISTICS:
	    {
	      if (l_IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
		  (sizeof (ULONG) * 4))
		{
		  l_Adapter->m_Tx =
		    ((PULONG) (p_IRP->AssociatedIrp.SystemBuffer))[0];
		  l_Adapter->m_Rx =
		    ((PULONG) (p_IRP->AssociatedIrp.SystemBuffer))[1];
		  l_Adapter->m_TxErr =
		    ((PULONG) (p_IRP->AssociatedIrp.SystemBuffer))[2];
		  l_Adapter->m_RxErr =
		    ((PULONG) (p_IRP->AssociatedIrp.SystemBuffer))[3];
		  p_IRP->IoStatus.Information = 1; // Simple boolean value
		}
	      else
		{
		  NOTE_ERROR (l_Adapter);
		  p_IRP->IoStatus.Status = l_Status = STATUS_INVALID_PARAMETER;
		}
	      
	      break;
	    }
#endif
	  case TAP_IOCTL_CONFIG_POINT_TO_POINT:
	    {
	      if (l_IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
		  (sizeof (IPADDR) * 2))
		{
		  MACADDR dest;
		  GenerateRelatedMAC (dest, l_Adapter->m_MAC);

		  NdisAcquireSpinLock (&l_Adapter->m_Lock);

		  l_Adapter->m_PointToPoint = TRUE;

		  l_Adapter->m_localIP =
		    ((IPADDR*) (p_IRP->AssociatedIrp.SystemBuffer))[0];
		  l_Adapter->m_remoteIP =
		    ((IPADDR*) (p_IRP->AssociatedIrp.SystemBuffer))[1];

		  COPY_MAC (l_Adapter->m_TapToUser.src, l_Adapter->m_MAC);
		  COPY_MAC (l_Adapter->m_TapToUser.dest, dest);
		  COPY_MAC (l_Adapter->m_UserToTap.src, dest);
		  COPY_MAC (l_Adapter->m_UserToTap.dest, l_Adapter->m_MAC);

		  l_Adapter->m_TapToUser.proto = l_Adapter->m_UserToTap.proto = htons (ETH_P_IP);

		  NdisReleaseSpinLock (&l_Adapter->m_Lock);

		  p_IRP->IoStatus.Information = 1; // Simple boolean value
		}
	      else
		{
		  NOTE_ERROR (l_Adapter);
		  p_IRP->IoStatus.Status = l_Status = STATUS_INVALID_PARAMETER;
		}
	      
	      break;
	    }

	  case TAP_IOCTL_SET_MEDIA_STATUS:
	    {
	      if (l_IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
		  (sizeof (ULONG) * 1))
		{
		  ULONG parm = ((PULONG) (p_IRP->AssociatedIrp.SystemBuffer))[0];
		  SetMediaStatus (l_Adapter, (BOOLEAN) parm);
		  p_IRP->IoStatus.Information = 1;
		}
	      else
		{
		  NOTE_ERROR (l_Adapter);
		  p_IRP->IoStatus.Status = l_Status = STATUS_INVALID_PARAMETER;
		}
	      break;
	    }

	  default:
	    {
	      NOTE_ERROR (l_Adapter);
	      p_IRP->IoStatus.Status = l_Status = STATUS_INVALID_PARAMETER;
	      break;
	    }
	  }

	IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
	break;
      }

      //===========================================================
      // User mode thread issued a read request on the tap device
      // If there are packets waiting to be read, then the request
      // will be satisfied here. If not, then the request will be
      // queued and satisfied by any packet that is not used to
      // satisfy requests ahead of it.
      //===========================================================
    case IRP_MJ_READ:
      {
	TapPacketPointer l_PacketBuffer;
	BOOLEAN pending = FALSE;

	// Save IRP-accessible copy of buffer length
	p_IRP->IoStatus.Information = l_IrpSp->Parameters.Read.Length;

	if (p_IRP->MdlAddress == NULL)
	  {
	    DEBUGP (("[%s] MdlAddress is NULL for IRP_MJ_READ\n",
		      l_Adapter->m_Name));
	    NOTE_ERROR (l_Adapter);
	    p_IRP->IoStatus.Status = l_Status = STATUS_INVALID_PARAMETER;
	    p_IRP->IoStatus.Information = 0;
	    IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
	    break;
	  }
	else if ((p_IRP->AssociatedIrp.SystemBuffer =
		  MmGetSystemAddressForMdlSafe
		  (p_IRP->MdlAddress, NormalPagePriority)) == NULL)
	  {
	    DEBUGP (("[%s] Could not map address in IRP_MJ_READ\n",
		      l_Adapter->m_Name));
	    NOTE_ERROR (l_Adapter);
	    p_IRP->IoStatus.Status = l_Status = STATUS_INSUFFICIENT_RESOURCES;
	    p_IRP->IoStatus.Information = 0;
	    IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
	    break;
	  }
	else if (!l_Adapter->m_InterfaceIsRunning)
	  {
	    DEBUGP (("[%s] Interface is down in IRP_MJ_READ\n",
		      l_Adapter->m_Name));
	    NOTE_ERROR (l_Adapter);
	    p_IRP->IoStatus.Status = l_Status = STATUS_UNSUCCESSFUL;
	    p_IRP->IoStatus.Information = 0;
	    IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
	    break;
	  }

	//==================================
	// Can we provide immediate service?
	//==================================

	l_PacketBuffer = NULL;

	NdisAcquireSpinLock (&l_Adapter->m_QueueLock);

	if (IS_UP (l_Adapter)
	    && QueueCount (l_Extension->m_PacketQueue)
	    && QueueCount (l_Extension->m_IrpQueue) == 0)
	  {
	    l_PacketBuffer = (TapPacketPointer)
	      QueuePop (l_Extension->m_PacketQueue);
	  }

	NdisReleaseSpinLock (&l_Adapter->m_QueueLock);

	if (l_PacketBuffer)
	  {
	    l_Status = CompleteIRP (l_Adapter,
				    p_IRP,
				    l_PacketBuffer,
				    IO_NO_INCREMENT);
	    break;
	  }

	//=============================
	// Attempt to pend read request
	//=============================

	NdisAcquireSpinLock (&l_Adapter->m_QueueLock);

	if (IS_UP (l_Adapter)
	    && QueuePush (l_Extension->m_IrpQueue, p_IRP) == (PIRP) p_IRP)
	  {
	    IoSetCancelRoutine (p_IRP, CancelIRPCallback);
	    l_Status = STATUS_PENDING;
	    IoMarkIrpPending (p_IRP);
	    pending = TRUE;
	  }

	NdisReleaseSpinLock (&l_Adapter->m_QueueLock);

	if (pending)
	  break;

	// Can't queue anymore IRP's
	DEBUGP (("[%s] TAP [%s] read IRP overrun\n",
		 l_Adapter->m_Name, l_Adapter->m_TapName));
	NOTE_ERROR (l_Adapter);
	p_IRP->IoStatus.Status = l_Status = STATUS_UNSUCCESSFUL;
	p_IRP->IoStatus.Information = 0;
	IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
	break;
      }

      //==============================================================
      // User mode issued a WriteFile request on the TAP file handle.
      // The request will always get satisfied here.  The call may
      // fail if there are too many pending packets (queue full).
      //==============================================================
    case IRP_MJ_WRITE:
      {
	if (p_IRP->MdlAddress == NULL)
	  {
	    DEBUGP (("[%s] MdlAddress is NULL for IRP_MJ_WRITE\n",
		      l_Adapter->m_Name));
	    NOTE_ERROR (l_Adapter);
	    p_IRP->IoStatus.Status = l_Status = STATUS_INVALID_PARAMETER;
	    p_IRP->IoStatus.Information = 0;
	  }
	else if ((p_IRP->AssociatedIrp.SystemBuffer =
		  MmGetSystemAddressForMdlSafe
		  (p_IRP->MdlAddress, NormalPagePriority)) == NULL)
	  {
	    DEBUGP (("[%s] Could not map address in IRP_MJ_WRITE\n",
		      l_Adapter->m_Name));
	    NOTE_ERROR (l_Adapter);
	    p_IRP->IoStatus.Status = l_Status = STATUS_INSUFFICIENT_RESOURCES;
	    p_IRP->IoStatus.Information = 0;
	  }
	else if (!l_Adapter->m_InterfaceIsRunning)
	  {
	    DEBUGP (("[%s] Interface is down in IRP_MJ_WRITE\n",
		      l_Adapter->m_Name));
	    NOTE_ERROR (l_Adapter);
	    p_IRP->IoStatus.Status = l_Status = STATUS_UNSUCCESSFUL;
	    p_IRP->IoStatus.Information = 0;
	  }
	else if (!l_Adapter->m_PointToPoint && ((l_IrpSp->Parameters.Write.Length) >= ETHERNET_HEADER_SIZE))
	  {
	    __try
	      {
		p_IRP->IoStatus.Information = l_IrpSp->Parameters.Write.Length;

		DEBUG_PACKET ("IRP_MJ_WRITE",
			      (unsigned char *) p_IRP->AssociatedIrp.SystemBuffer,
			      l_IrpSp->Parameters.Write.Length);

		NdisMEthIndicateReceive
		  (l_Adapter->m_MiniportAdapterHandle,
		   (NDIS_HANDLE) l_Adapter,
		   (unsigned char *) p_IRP->AssociatedIrp.SystemBuffer,
		   ETHERNET_HEADER_SIZE,
		   (unsigned char *) p_IRP->AssociatedIrp.SystemBuffer +
		   ETHERNET_HEADER_SIZE,
		   l_IrpSp->Parameters.Write.Length - ETHERNET_HEADER_SIZE,
		   l_IrpSp->Parameters.Write.Length - ETHERNET_HEADER_SIZE);
		
		NdisMEthIndicateReceiveComplete (l_Adapter->m_MiniportAdapterHandle);

		p_IRP->IoStatus.Status = l_Status = STATUS_SUCCESS;
	      }
	      __except (EXCEPTION_EXECUTE_HANDLER)
	      {
		DEBUGP (("[%s] NdisMEthIndicateReceive failed in IRP_MJ_WRITE\n",
			 l_Adapter->m_Name));
		NOTE_ERROR (l_Adapter);
		p_IRP->IoStatus.Status = l_Status = STATUS_UNSUCCESSFUL;
		p_IRP->IoStatus.Information = 0;
	      }
	  }
	else if (l_Adapter->m_PointToPoint && ((l_IrpSp->Parameters.Write.Length) >= IP_HEADER_SIZE))
	  {
	    __try
	      {
		p_IRP->IoStatus.Information = l_IrpSp->Parameters.Write.Length;

		NdisMEthIndicateReceive
		  (l_Adapter->m_MiniportAdapterHandle,
		   (NDIS_HANDLE) l_Adapter,
		   (unsigned char *) &l_Adapter->m_UserToTap,
		   sizeof (l_Adapter->m_UserToTap),
		   (unsigned char *) p_IRP->AssociatedIrp.SystemBuffer,
		   l_IrpSp->Parameters.Write.Length,
		   l_IrpSp->Parameters.Write.Length);

		NdisMEthIndicateReceiveComplete (l_Adapter->m_MiniportAdapterHandle);

		p_IRP->IoStatus.Status = l_Status = STATUS_SUCCESS;
	      }
	      __except (EXCEPTION_EXECUTE_HANDLER)
	      {
		DEBUGP (("[%s] NdisMEthIndicateReceive failed in IRP_MJ_WRITE (P2P)\n",
			 l_Adapter->m_Name));
		NOTE_ERROR (l_Adapter);
		p_IRP->IoStatus.Status = l_Status = STATUS_UNSUCCESSFUL;
		p_IRP->IoStatus.Information = 0;
	      }
	  }
	else
	  {
	    DEBUGP (("[%s] Bad buffer size in IRP_MJ_WRITE, len=%d\n",
			  l_Adapter->m_Name,
			  l_IrpSp->Parameters.Write.Length));
	    NOTE_ERROR (l_Adapter);
	    p_IRP->IoStatus.Information = 0;	// ETHERNET_HEADER_SIZE;
	    p_IRP->IoStatus.Status = l_Status = STATUS_BUFFER_TOO_SMALL;
	  }

#ifndef NEED_TAP_IOCTL_SET_STATISTICS
	if (l_Status == STATUS_SUCCESS)
	  INCREMENT_STAT (l_Adapter->m_Rx);
	else
	  INCREMENT_STAT (l_Adapter->m_RxErr);
#endif
	IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
	break;
      }

      //--------------------------------------------------------------
      //   User mode thread has called CreateFile() on the tap device
      //--------------------------------------------------------------
    case IRP_MJ_CREATE:
      {
	if (l_Adapter->m_TapIsRunning
#ifdef DISABLE_DEVICE_SHARING
	    && l_Adapter->m_TapOpens < 1
#endif
	    )
	  {
	    BOOLEAN first_open;

	    DEBUGP
	      (("[%s] [TAP] release [%d.%d] open request (m_TapOpens=%d)\n",
	       l_Adapter->m_Name, TAP_DRIVER_MAJOR_VERSION,
	       TAP_DRIVER_MINOR_VERSION, l_Adapter->m_TapOpens));

	    NdisAcquireSpinLock (&l_Adapter->m_Lock);
	    ++l_Adapter->m_TapOpens;
	    first_open = (l_Adapter->m_TapOpens == 1);
	    if (first_open)
	      ResetPointToPointMode (l_Adapter);
	    NdisReleaseSpinLock (&l_Adapter->m_Lock);

#ifdef SET_MEDIA_STATUS_ON_OPEN
	    if (first_open)
	      SetMediaStatus (l_Adapter, TRUE);
#endif

	    INCREMENT_STAT (l_Adapter->m_NumTapOpens);
	    p_IRP->IoStatus.Status = l_Status = STATUS_SUCCESS;
	    p_IRP->IoStatus.Information = 0;
	  }
	else
	  {
	    DEBUGP (("[%s] TAP is presently unavailable (m_TapOpens=%d)\n",
		      l_Adapter->m_Name, l_Adapter->m_TapOpens));
	    NOTE_ERROR (l_Adapter);
	    p_IRP->IoStatus.Status = l_Status = STATUS_UNSUCCESSFUL;
	    p_IRP->IoStatus.Information = 0;
	  }
	
	IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
	break;
      }
      
      //-----------------------------------------------------------
      //  User mode thread called CloseHandle() on the tap device
      //-----------------------------------------------------------
    case IRP_MJ_CLOSE:
      {
	BOOLEAN fully_closed = FALSE;
	
	DEBUGP (("[%s] [TAP] release [%d.%d] close request\n",
		  l_Adapter->m_Name, TAP_DRIVER_MAJOR_VERSION,
		  TAP_DRIVER_MINOR_VERSION));

	NdisAcquireSpinLock (&l_Adapter->m_Lock);
	if (l_Adapter->m_TapOpens)
	  {
	    --l_Adapter->m_TapOpens;
	    if (l_Adapter->m_TapOpens == 0)
	      {
		fully_closed = TRUE;
		ResetPointToPointMode (l_Adapter);
	      }
	  }
	NdisReleaseSpinLock (&l_Adapter->m_Lock);

	if (fully_closed)
	  {
	    FlushQueues (l_Adapter);
	    SetMediaStatus (l_Adapter, FALSE);
	  }

	IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
	break;
      }

      //-----------------------------------------------------------
      // Something screwed up if it gets here ! It won't die, though
      //-----------------------------------------------------------
    default:
      {
	//NOTE_ERROR (l_Adapter);
	p_IRP->IoStatus.Status = l_Status = STATUS_UNSUCCESSFUL;
	IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
	break;
      }
    }

  return l_Status;
}

//=============================================================
// CompleteIRP is normally called with an adapter -> userspace
// network packet and an IRP (Pending I/O request) from userspace.
//
// The IRP will normally represent a queued overlapped read
// operation from userspace that is in a wait state.
//
// Use the ethernet packet to satisfy the IRP.
//=============================================================

NTSTATUS
CompleteIRP (TapAdapterPointer p_Adapter,
             IN PIRP p_IRP,
	     IN TapPacketPointer p_PacketBuffer,
	     IN CCHAR PriorityBoost)
{
  NTSTATUS l_Status = STATUS_UNSUCCESSFUL;

  int offset;
  int len;

  MYASSERT (p_IRP);
  MYASSERT (p_PacketBuffer);

  IoSetCancelRoutine (p_IRP, NULL);  // Disable cancel routine

  // While p_PacketBuffer always contains a
  // full ethernet packet, including the
  // ethernet header, in point-to-point mode,
  // we only want to return the IPv4
  // component.

  if (p_PacketBuffer->m_SizeFlags & TP_POINT_TO_POINT)
    {
      offset = ETHERNET_HEADER_SIZE;
      len = (int) (p_PacketBuffer->m_SizeFlags & TP_SIZE_MASK) - ETHERNET_HEADER_SIZE;
    }
  else
    {
      offset = 0;
      len = (p_PacketBuffer->m_SizeFlags & TP_SIZE_MASK);
    }

  if (len < 0 || (int) p_IRP->IoStatus.Information < len)
    {
      p_IRP->IoStatus.Information = 0;
      p_IRP->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
      NOTE_ERROR (p_Adapter);
    }
  else
    {
      p_IRP->IoStatus.Information = len;
      p_IRP->IoStatus.Status = l_Status = STATUS_SUCCESS;

      __try
	{
	  NdisMoveMemory (p_IRP->AssociatedIrp.SystemBuffer,
			  p_PacketBuffer->m_Data + offset,
			  len);
	}
      __except (EXCEPTION_EXECUTE_HANDLER)
	{
	  NOTE_ERROR (p_Adapter);
	  p_IRP->IoStatus.Status = STATUS_UNSUCCESSFUL;
	  p_IRP->IoStatus.Information = 0;
	}
    }

  __try
    {
      NdisFreeMemory (p_PacketBuffer,
		      TAP_PACKET_SIZE (p_PacketBuffer->m_SizeFlags & TP_SIZE_MASK),
		      0);
    }
  __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
  
  if (l_Status == STATUS_SUCCESS)
    {
      IoCompleteRequest (p_IRP, PriorityBoost);
    }
  else
    IoCompleteRequest (p_IRP, IO_NO_INCREMENT);

  return l_Status;
}

//==============================================
// IRPs get cancelled for a number of reasons.
//
// The TAP device could be closed by userspace
// when there are still pending read operations.
//
// The user could disable the TAP adapter in the
// network connections control panel, while the
// device is still open by a process.
//==============================================
VOID
CancelIRPCallback (IN PDEVICE_OBJECT p_DeviceObject,
		   IN PIRP p_IRP)
{
  CancelIRP (p_DeviceObject, p_IRP, TRUE);
}

VOID
CancelIRP (IN PDEVICE_OBJECT p_DeviceObject,
	   IN PIRP p_IRP,
	   BOOLEAN callback)
{
  TapExtensionPointer l_Extension =
    (TapExtensionPointer) p_DeviceObject->DeviceExtension;

  BOOLEAN exists = FALSE;

  MYASSERT (p_IRP);

  if (!l_Extension->m_halt)
    {
      NdisAcquireSpinLock (&l_Extension->m_Adapter->m_Lock);
      exists = (QueueExtract (l_Extension->m_IrpQueue, p_IRP) == p_IRP);
      NdisReleaseSpinLock (&l_Extension->m_Adapter->m_Lock);
    }
  else
    exists = TRUE;

  if (exists)
    {
      IoSetCancelRoutine (p_IRP, NULL);
      p_IRP->IoStatus.Status = STATUS_CANCELLED;
      p_IRP->IoStatus.Information = 0;
    }
     
  if (callback)
    IoReleaseCancelSpinLock (p_IRP->CancelIrql);

  if (exists)
    IoCompleteRequest (p_IRP, IO_NO_INCREMENT);
}

//====================================
// Exhaust packet and IRP queues.
//====================================
VOID
FlushQueues (TapAdapterPointer p_Adapter)
{
  PIRP l_IRP;
  TapPacketPointer l_PacketBuffer;
  int n_IRP=0, n_Packet=0;
  TapExtensionPointer l_Extension;

  MYASSERT (p_Adapter);
  MYASSERT (p_Adapter->m_TapDevice);

  l_Extension = (TapExtensionPointer)
    p_Adapter->m_TapDevice->DeviceExtension;

  while (TRUE)
    {
      NdisAcquireSpinLock (&p_Adapter->m_QueueLock);
      l_IRP = QueuePop (l_Extension->m_IrpQueue);
      NdisReleaseSpinLock (&p_Adapter->m_QueueLock);
      if (l_IRP)
	{
	  ++n_IRP;
	  CancelIRP (p_Adapter->m_TapDevice, l_IRP, FALSE);
	}
      else
	break;
    }

  while (TRUE)
    {
      NdisAcquireSpinLock (&p_Adapter->m_QueueLock);
      l_PacketBuffer = QueuePop (l_Extension->m_PacketQueue);
      NdisReleaseSpinLock (&p_Adapter->m_QueueLock);
      if (l_PacketBuffer)
	{
	  ++n_Packet;
	  MemFree (l_PacketBuffer, TAP_PACKET_SIZE (l_PacketBuffer->m_SizeFlags & TP_SIZE_MASK));
	}
      else
	break;
    }

  DEBUGP ((
	   "[%s] [TAP] FlushQueues n_IRP=[%d,%d,%d] n_Packet=[%d,%d,%d]\n",
	   p_Adapter->m_Name,
	   n_IRP,
	   l_Extension->m_IrpQueue->max_size,
	   IRP_QUEUE_SIZE,
	   n_Packet,
	   l_Extension->m_PacketQueue->max_size,
	   PACKET_QUEUE_SIZE
	   ));
}

//===================================================
// Tell Windows whether the TAP device should be
// considered "connected" or "disconnected".
//===================================================
VOID
SetMediaStatus (TapAdapterPointer p_Adapter, BOOLEAN state)
{
  if (p_Adapter->m_MediaState != state && !p_Adapter->m_MediaStateAlwaysConnected)
    {
      if (state)
	NdisMIndicateStatus (p_Adapter->m_MiniportAdapterHandle,
			     NDIS_STATUS_MEDIA_CONNECT, NULL, 0);
      else
	NdisMIndicateStatus (p_Adapter->m_MiniportAdapterHandle,
			     NDIS_STATUS_MEDIA_DISCONNECT, NULL, 0);

      NdisMIndicateStatusComplete (p_Adapter->m_MiniportAdapterHandle);
      p_Adapter->m_MediaState = state;
    }
}

//===================================================
// Generate a return ARP message for
// point-to-point IPv4 mode.
//===================================================

void ProcessARP (TapAdapterPointer p_Adapter, const PARP_PACKET src)
{
  //-----------------------------------------------
  // Is this the kind of packet we are looking for?
  //-----------------------------------------------
  if (src->m_Proto == htons (ETH_P_ARP)
      && MAC_EQUAL (src->m_MAC_Source, p_Adapter->m_MAC)
      && MAC_EQUAL (src->m_ARP_MAC_Source, p_Adapter->m_MAC)
      && MAC_EQUAL (src->m_MAC_Destination, p_Adapter->m_MAC_Broadcast)
      && src->m_ARP_Operation == htons (ARP_REQUEST)
      && src->m_MAC_AddressType == htons (MAC_ADDR_TYPE)
      && src->m_MAC_AddressSize == sizeof (MACADDR)
      && src->m_PROTO_AddressType == htons (ETH_P_IP)
      && src->m_PROTO_AddressSize == sizeof (IPADDR)
      && src->m_ARP_IP_Source == p_Adapter->m_localIP
      && src->m_ARP_IP_Destination == p_Adapter->m_remoteIP)
    {
      ARP_PACKET arp;
      NdisZeroMemory (&arp, sizeof (arp));

      //----------------------------------------------
      // Initialize ARP reply fields
      //----------------------------------------------
      arp.m_Proto = htons (ETH_P_ARP);
      arp.m_MAC_AddressType = htons (MAC_ADDR_TYPE);
      arp.m_PROTO_AddressType = htons (ETH_P_IP);
      arp.m_MAC_AddressSize = sizeof (MACADDR);
      arp.m_PROTO_AddressSize = sizeof (IPADDR);
      arp.m_ARP_Operation = htons (ARP_REPLY);

      //----------------------------------------------
      // ARP addresses
      //----------------------------------------------      
      COPY_MAC (arp.m_MAC_Source, p_Adapter->m_TapToUser.dest);
      COPY_MAC (arp.m_MAC_Destination, p_Adapter->m_TapToUser.src);
      COPY_MAC (arp.m_ARP_MAC_Source, p_Adapter->m_TapToUser.dest);
      COPY_MAC (arp.m_ARP_MAC_Destination, p_Adapter->m_TapToUser.src);
      arp.m_ARP_IP_Source = p_Adapter->m_remoteIP;
      arp.m_ARP_IP_Destination = p_Adapter->m_localIP;

      DEBUG_PACKET ("ProcessARP",
		    (unsigned char *) &arp,
		    sizeof (arp));

      __try
	{
	  //------------------------------------------------------------
	  // NdisMEthIndicateReceive and NdisMEthIndicateReceiveComplete
	  // could potentially be called reentrantly both here and in
	  // TapDeviceHook/IRP_MJ_WRITE.
	  //
	  // The DDK docs imply that this is okay.
	  //------------------------------------------------------------
	  NdisMEthIndicateReceive
	    (p_Adapter->m_MiniportAdapterHandle,
	     (NDIS_HANDLE) p_Adapter,
	     (unsigned char *) &arp,
	     ETHERNET_HEADER_SIZE,
	     ((unsigned char *) &arp) + ETHERNET_HEADER_SIZE,
	     sizeof (arp) - ETHERNET_HEADER_SIZE,
	     sizeof (arp) - ETHERNET_HEADER_SIZE);
		
	  NdisMEthIndicateReceiveComplete (p_Adapter->m_MiniportAdapterHandle);
	}
      __except (EXCEPTION_EXECUTE_HANDLER)
	{
	  DEBUGP (("[%s] NdisMEthIndicateReceive failed in ProcessARP\n",
		   p_Adapter->m_Name));
	  NOTE_ERROR (p_Adapter);
	}
    }
}

//===================================================================
// Go back to default TAP mode from Point-To-Point mode
//===================================================================
void ResetPointToPointMode (TapAdapterPointer p_Adapter)
{
  p_Adapter->m_PointToPoint = FALSE;
  p_Adapter->m_localIP = 0;
  p_Adapter->m_remoteIP = 0;
  NdisZeroMemory (&p_Adapter->m_TapToUser, sizeof (p_Adapter->m_TapToUser));
  NdisZeroMemory (&p_Adapter->m_UserToTap, sizeof (p_Adapter->m_UserToTap));
}

//===================================================================
//                             Dispatch Table Managemement
//===================================================================
VOID
HookDispatchFunctions ()
{
  unsigned long l_Index;

  //==============================================================
  // Save original NDIS dispatch functions and override with ours
  //==============================================================
  if (!g_DispatchFunctionsHooked)
    for (l_Index = 0, g_DispatchFunctionsHooked = 1;
	 l_Index <= IRP_MJ_MAXIMUM_FUNCTION; ++l_Index)
      {
	g_DispatchHook[l_Index] = g_TapDriverObject->MajorFunction[l_Index];
	g_TapDriverObject->MajorFunction[l_Index] = TapDeviceHook;
      }
}

//======================================================================
//                                    End of Source
//======================================================================
