/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "ddk.h"
#include "driver.h"

#include <colinux/kernel/manager.h>
#include <colinux/os/current/monitor.h>
#include <colinux/os/current/ioctl.h>

void co_monitor_os_wait_exclusiveness(co_monitor_t *cmon)
{
	KeWaitForSingleObject(&cmon->osdep->exclusiveness, 
			      UserRequest, KernelMode, FALSE, NULL);
}

void co_debug_stop_hook1(co_monitor_t *cmon)
{
	KeWaitForSingleObject(&cmon->osdep->debugstop, UserRequest, KernelMode, FALSE, NULL);
}

void co_debug_continue_hook1(co_monitor_t *cmon)
{
	KeSetEvent(&cmon->osdep->debugstop, 1, FALSE);
}

NTSTATUS 
NTAPI
co_manager_dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION  irpStack;
	PVOID               ioBuffer;
	ULONG               inputBufferLength;
	ULONG               outputBufferLength;
	ULONG               ioControlCode;
	NTSTATUS            ntStatus;
	co_manager_t             *co_manager;
	co_rc_t             rc;
	co_manager_ioctl_t        ioctl;

	Irp->IoStatus.Status      = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	irpStack = IoGetCurrentIrpStackLocation (Irp);
	co_manager = (co_manager_t *)DeviceObject->DeviceExtension;

	ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
	inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (irpStack->MajorFunction) {
    
	case IRP_MJ_DEVICE_CONTROL: 
		ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

		if (CO_GET_IOCTL_TYPE(ioControlCode) != CO_DRIVER_TYPE) {
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER; 
			break;
		}

		if (CO_GET_IOCTL_MTYPE(ioControlCode) != METHOD_BUFFERED) {
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER; 
			break;
		}

		if (CO_GET_IOCTL_ACCESS(ioControlCode) != FILE_ANY_ACCESS) {
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER; 
			break;
		}

		ioctl = (co_manager_ioctl_t)CO_GET_IOCTL_METHOD(ioControlCode);

		rc = co_manager_ioctl(co_manager, ioctl, ioBuffer, 
				      inputBufferLength,
				      outputBufferLength,
				      &Irp->IoStatus.Information);

		if (!CO_OK(rc))
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER; 

		break;
	}

	ntStatus = Irp->IoStatus.Status;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return ntStatus;
}

VOID 
NTAPI
co_manager_unload(IN PDRIVER_OBJECT DriverObject)
{
	WCHAR               deviceLinkBuffer[]  = L"\\DosDevices\\"CO_DRIVER_NAME;
	UNICODE_STRING      deviceLinkUnicodeString;

	RtlInitUnicodeString(&deviceLinkUnicodeString, deviceLinkBuffer);

	IoDeleteSymbolicLink(&deviceLinkUnicodeString);
	IoDeleteDevice (DriverObject->DeviceObject);

	co_debug("Driver Unloaded");
}

NTSTATUS 
NTAPI
DriverEntry( 
	IN PDRIVER_OBJECT  DriverObject,
	IN PUNICODE_STRING  RegistryPath 
	)
{
	PDEVICE_OBJECT deviceObject = NULL;
	NTSTATUS ntStatus;
	WCHAR deviceNameBuffer[]  = L"\\Device\\" CO_DRIVER_NAME;
	UNICODE_STRING deviceNameUnicodeString;
	WCHAR deviceLinkBuffer[]  = L"\\DosDevices\\" CO_DRIVER_NAME;
	UNICODE_STRING deviceLinkUnicodeString;
	co_manager_t *co_manager;

	RtlInitUnicodeString (&deviceNameUnicodeString, deviceNameBuffer);

	ntStatus = IoCreateDevice (DriverObject,
				   sizeof(co_manager_t),
				   &deviceNameUnicodeString,
				   CO_DRIVER_TYPE,
				   0,
				   FALSE,
				   &deviceObject);

	if (!NT_SUCCESS(ntStatus)) 
		return ntStatus;

	co_manager = (co_manager_t *)deviceObject->DeviceExtension;
	co_manager->state = CO_MANAGER_STATE_NOT_INITIALIZED;

	RtlInitUnicodeString (&deviceLinkUnicodeString, deviceLinkBuffer);
	ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
					 &deviceNameUnicodeString);

	if (!NT_SUCCESS(ntStatus)) {
		IoDeleteDevice (deviceObject);
		return ntStatus;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE]         =
	DriverObject->MajorFunction[IRP_MJ_CLOSE]          =
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = co_manager_dispatch;
	DriverObject->DriverUnload                         = co_manager_unload;

	co_debug("Driver Loaded");

	return STATUS_SUCCESS;
}
