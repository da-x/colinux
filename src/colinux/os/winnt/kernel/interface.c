/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
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
	case IRP_MJ_CREATE: 
		irpStack->FileObject->FsContext = NULL;
		break;

	case IRP_MJ_CLOSE: 
		break;

	case IRP_MJ_CLEANUP: {
		co_manager_cleanup(co_manager, &irpStack->FileObject->FsContext);
		break;
	}
    
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
				      &Irp->IoStatus.Information,
				      (void **)&irpStack->FileObject->FsContext);

		if (!CO_OK(rc))
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER; 

		break;

	}

	ntStatus = Irp->IoStatus.Status;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return ntStatus;
}

static
VOID 
NTAPI
driver_unload(IN PDRIVER_OBJECT DriverObject)
{
	WCHAR               deviceLinkBuffer[]  = L"\\DosDevices\\"CO_DRIVER_NAME;
	UNICODE_STRING      deviceLinkUnicodeString;
	co_manager_t        *manager;

	
	manager = DriverObject->DeviceObject->DeviceExtension;

        /* 
	 *  TODO:
	 *
	 *  Make sure that there aren't any users of the driver
	 *  before we continue and remove it.
	 *
	 *  while (manager->monitors_count != 0) {
	 *
	 *  }
	 */

	co_manager_unload(manager);

	RtlInitUnicodeString(&deviceLinkUnicodeString, deviceLinkBuffer);

	IoDeleteSymbolicLink(&deviceLinkUnicodeString);
	IoDeleteDevice(DriverObject->DeviceObject);
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
	DriverObject->MajorFunction[IRP_MJ_CLEANUP]        =
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = co_manager_dispatch;
	DriverObject->DriverUnload                         = driver_unload;

	co_manager_load(co_manager);

	return STATUS_SUCCESS;
}
