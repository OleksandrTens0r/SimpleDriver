#include "ntddk.h"

#define IOCTL_DEVICE_SEND CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_DEVICE_REC CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)

UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\myfirstdevice");
UNICODE_STRING SymbolLinkName = RTL_CONSTANT_STRING(L"\\??\\HelloBenderDevice");

PDEVICE_OBJECT DeviceObject = NULL;

VOID Unload(PDRIVER_OBJECT DriverObject)
{
	IoDeleteSymbolicLink(&SymbolLinkName);
	IoDeleteDevice(DeviceObject);
	KdPrint(("Driver Unload"));
}

NTSTATUS DriverIRPHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION irp_io_stack_location = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;

	switch (irp_io_stack_location->MajorFunction)
	{
	case IRP_MJ_CREATE:
		KdPrint(("create request \r\n"));
		break;
	case IRP_MJ_CLOSE:
		KdPrint(("close request \r\n"));
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
		break;
	}

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS DispatchDevControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION stack_location = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;

	ULONG returnLength = 0;

	PVOID buffer = Irp->AssociatedIrp.SystemBuffer;

	ULONG in_length = stack_location->Parameters.DeviceIoControl.InputBufferLength;
	ULONG out_length = stack_location->Parameters.DeviceIoControl.OutputBufferLength;

	PWCHAR message = L"my message for driver user";

	switch (stack_location->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_DEVICE_SEND:
		// here I must check buffer is \n present  
		KdPrint(("send data is %ws \r\n", buffer));
		returnLength = (wcsnlen(buffer, 511) + 1) * 2;
		break;
	case IOCTL_DEVICE_REC:
		wcsncpy(buffer, message, 511);
		returnLength = (wcsnlen(buffer, 511) + 1) * 2;
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = returnLength;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;

	DriverObject->DriverUnload = Unload;

	status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("creating device failed -> my message \r\n"));
		return status;
	}

	status = IoCreateSymbolicLink(&SymbolLinkName, &DeviceName);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("create symbol link failed -> my message \r\n"));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		DriverObject->MajorFunction[i] = DriverIRPHandler; 
	}

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDevControl;

	KdPrint(("Driver loaded cool!!"));
	return status;
}
