#include "stdafx.h"

#include "Information.h"
#include "Log.h"
#include "Routines.h"



void DriverUnload(PDRIVER_OBJECT pDriver);
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegistryPath)
{
	UNREFERENCED_PARAMETER(pRegistryPath);

	NTSTATUS ntDriverEntryStatus	= STATUS_SUCCESS;
	PDEVICE_OBJECT pDevice			= nullptr;

	do
	{
		//Verify windows version (1709)
		RTL_OSVERSIONINFOW OsInfo = {};
		ntDriverEntryStatus = RtlGetVersion(&OsInfo);
		if (NT_ERROR(ntDriverEntryStatus))
		{
			LogMessage(L"Failed to get current os version.", ErrorNotification);
			break;
		}
		if (OsInfo.dwMajorVersion != 10 || OsInfo.dwMinorVersion != 0 || OsInfo.dwBuildNumber != 16299)
		{
			ntDriverEntryStatus = STATUS_INTERNAL_ERROR;
			LogMessage(L"Unsupported windows version.", ErrorNotification);
			break;
		}

		ntDriverEntryStatus = IoCreateDevice(pDriver, NULL, &ustDeviceName, FILE_DEVICE_UNKNOWN, NULL, FALSE, &pDevice);
		if (NT_ERROR(ntDriverEntryStatus))
		{
			LogMessage(L"Failed to create communication device.", ErrorNotification);
			break;
		}

		ntDriverEntryStatus = IoCreateUnprotectedSymbolicLink(&ustSymbolicLink, &ustDeviceName);
		if (NT_ERROR(ntDriverEntryStatus))
		{
			LogMessage(L"Failed to create symbolic link.", ErrorNotification);
			break;
		}

		pDriver->DriverUnload							= DriverUnload;
		pDriver->MajorFunction[IRP_MJ_CREATE]			= pDriver->MajorFunction[IRP_MJ_CLOSE] = CreateCloseDeviceRoutine;
		pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL]	= DeviceIoControlRoutine;
	} while (false);
	
	if (NT_ERROR(ntDriverEntryStatus))
	{
		LogMessage(L"DriverEntry has finished with a error(s)", ErrorNotification);
	}
	else
	{
		LogMessage(L"DriverEntry has finished with no errors.", GenericNotificaton);
	}

	return ntDriverEntryStatus;
}



void DriverUnload(PDRIVER_OBJECT pDriver)
{
	NTSTATUS ntDriverUnloadStatus = IoDeleteSymbolicLink(&ustSymbolicLink);
	if (NT_ERROR(ntDriverUnloadStatus))
	{
		LogMessage(L"Failed to delete symbolic link.", ErrorNotification);
	}

	if (pDriver->DeviceObject)
	{
		IoDeleteDevice(pDriver->DeviceObject);
	}

	if (NT_ERROR(ntDriverUnloadStatus))
	{
		LogMessage(L"DriverUnload ahs finished with a error(s)", ErrorNotification);
	}
	else
	{
		LogMessage(L"DriverUnload has finished with no errors.", GenericNotificaton);
	}
}