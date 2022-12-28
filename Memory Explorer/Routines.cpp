#include "stdafx.h"

#include "Log.h"
#include "Typedefs.h"
#include "DeviceIoControlCodes.h"
#include "PageTable.h"
#include "Memory.h"
#include "CommunicationStructures.h"
#include "KernelStructures.h"



NTSTATUS CreateCloseDeviceRoutine(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);

	LogMessage(L"Create/Close device routine called.", GenericNotificaton);

	pIrp->IoStatus.Status		= STATUS_SUCCESS;
	pIrp->IoStatus.Information	= NULL;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}



NTSTATUS DeviceIoControlRoutine(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevice);
	
	NTSTATUS ntDeviceIoControlRoutineStatus = STATUS_SUCCESS;
	do
	{
		PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
		if (!pIrpStack)
		{
			LogMessage(L"Failed to get current irp stack address.", ErrorNotification);
			break;
		}


		switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode)
		{

		case IOCTL_LookupPage:
		{
			pIrp->IoStatus.Information = sizeof(PageTableEntry);

			if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(PageLookupRequest) || pIrpStack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(PageTableEntry))
			{
				LogMessage(L"Invalid parameter size for IOCTL_LookupPage request.", ErrorNotification);
				ntDeviceIoControlRoutineStatus = STATUS_INVALID_PARAMETER;
				break;
			}

			PageLookupRequest* pLookup = reinterpret_cast<PageLookupRequest*>(pIrp->AssociatedIrp.SystemBuffer);
			if (!pLookup)
			{
				LogMessage(L"Null paramer for IOCTL_LookupPage request.", ErrorNotification);
				ntDeviceIoControlRoutineStatus = STATUS_INVALID_PARAMETER;
				break;
			}

			PEPROCESS pTargetProcess = nullptr;
			ntDeviceIoControlRoutineStatus = PsLookupProcessByProcessId(ULongToHandle(pLookup->dwProcessId), &pTargetProcess);
			if (NT_ERROR(ntDeviceIoControlRoutineStatus))
			{
				LogMessage(L"Provided process id is not valid for IOCTL_LookupPage request.", ErrorNotification);
				break;
			}

			__try
			{
				ULONG64 paTargetPML4 = reinterpret_cast<iEPROCESS*>(pTargetProcess)->Pcb.DirectoryTableBase;
				if (!paTargetPML4)
				{
					LogMessage(L"Target process does not have a valid PML4 for IOCTL_LookupPage request.", ErrorNotification);
					ntDeviceIoControlRoutineStatus = STATUS_INTERNAL_ERROR;
					break;
				}

				PageTableEntry Entry = GetPageEntry(pLookup->pAddress, paTargetPML4, false);
				if (Entry.ullPhysicalAddress == NULL && Entry.ullPTE == NULL && Entry.ullVirtualAdddress == NULL)
				{
					LogMessage(L"An error occured when attempting to traverse the page table for IOCTL_LookupPage request.", ErrorNotification);
					ntDeviceIoControlRoutineStatus = STATUS_INTERNAL_ERROR;
					break;
				}

				if (!(pIrp->AssociatedIrp.SystemBuffer))
				{
					LogMessage(L"BUFFERED_IO buffer was corrupted for IOCTL_LookupPage request.", ErrorNotification);
					ntDeviceIoControlRoutineStatus = STATUS_INTERNAL_ERROR;
					break;
				}
				
				*(reinterpret_cast<PageTableEntry*>(pIrp->AssociatedIrp.SystemBuffer)) = Entry;
			}
			__finally
			{
				if (pTargetProcess)
				{
					ObDereferenceObject(pTargetProcess);
				}
			}
			
			break;
		}



		case IOCTL_ModifyPage:
		{
			pIrp->IoStatus.Information = NULL;

			if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(PageModificationRequest) || pIrpStack->Parameters.DeviceIoControl.OutputBufferLength != NULL)
			{
				LogMessage(L"Invalid parameter size for IOCTL_ModifyPage request.", ErrorNotification);
				ntDeviceIoControlRoutineStatus = STATUS_INVALID_PARAMETER;
				break;
			}

			PageModificationRequest* pModify = reinterpret_cast<PageModificationRequest*>(pIrp->AssociatedIrp.SystemBuffer);
			if (!pModify)
			{
				LogMessage(L"Null paramer for IOCTL_ModifyPage request.", ErrorNotification);
				ntDeviceIoControlRoutineStatus = STATUS_INVALID_PARAMETER;
				break;
			}

			PEPROCESS pTargetProcess = nullptr;
			ntDeviceIoControlRoutineStatus = PsLookupProcessByProcessId(ULongToHandle(pModify->dwProcessId), &pTargetProcess);
			if (NT_ERROR(ntDeviceIoControlRoutineStatus))
			{
				LogMessage(L"Provided process id is not valid for IOCTL_ModifyPage request.", ErrorNotification);
				break;
			}

			__try
			{
				ULONG64 paTargetPML4 = reinterpret_cast<iEPROCESS*>(pTargetProcess)->Pcb.DirectoryTableBase;
				if (!paTargetPML4)
				{
					LogMessage(L"Target process does not have a valid PML4 for IOCTL_ModifyPage request.", ErrorNotification);
					ntDeviceIoControlRoutineStatus = STATUS_INTERNAL_ERROR;
					break;
				}

				PageTableEntry Entry = GetPageEntry(pModify->pAddress, paTargetPML4, true, &(pModify->Flags));
				if (Entry.ullPhysicalAddress != 1 && Entry.ullPTE != 1 && Entry.ullVirtualAdddress != 1)
				{
					LogMessage(L"An error occured when attempting to traverse the page table for IOCTL_ModifyPage request.", ErrorNotification);
					ntDeviceIoControlRoutineStatus = STATUS_INTERNAL_ERROR;
					break;
				}
			}
			__finally
			{
				ObDereferenceObject(pTargetProcess);
			}

			break;
		}



		case IOCTL_ReadMemory:
		{
			if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(ReadRequest))
			{
				LogMessage(L"Invalid parameter size for IOCTL_ReadMemory request.", ErrorNotification);
				ntDeviceIoControlRoutineStatus = STATUS_INVALID_PARAMETER;
				break;
			}

			ReadRequest* pRead = reinterpret_cast<ReadRequest*>(pIrp->AssociatedIrp.SystemBuffer);
			if (!pRead)
			{
				LogMessage(L"Null paramer for IOCTL_ReadMemory request.", ErrorNotification);
				ntDeviceIoControlRoutineStatus = STATUS_INVALID_PARAMETER;
				break;
			}

			if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength != pRead->ullSize)
			{
				LogMessage(L"Invalid parameter size for IOCTL_ReadMemory request.", ErrorNotification);
				ntDeviceIoControlRoutineStatus = STATUS_INVALID_PARAMETER;
				break;
			}

			pIrp->IoStatus.Information = pRead->ullSize;

			PEPROCESS pTargetProcess = nullptr;
			ntDeviceIoControlRoutineStatus = PsLookupProcessByProcessId(UlongToHandle(pRead->dwProcessId), &pTargetProcess);
			if (NT_ERROR(ntDeviceIoControlRoutineStatus))
			{
				LogMessage(L"Provided process id is not valid for IOCTL_ReadMemory request.", ErrorNotification);
				break;
			}

			__try
			{
				if (!ReadMemoryThroughPageTable(pRead->pAddress, reinterpret_cast<iEPROCESS*>(pTargetProcess)->Pcb.DirectoryTableBase, pIrp->AssociatedIrp.SystemBuffer, pRead->ullSize))
				{
					LogMessage(L"An error occured when attempting to copy memory for IOCTL_ReadMemory request.", ErrorNotification);
					ntDeviceIoControlRoutineStatus = STATUS_INTERNAL_ERROR;
					break;
				}
			}
			__finally
			{
				if (pTargetProcess)
				{
					ObDereferenceObject(pTargetProcess);
				}
			}

			break;
		}



		case IOCTL_WriteMemory:
		{
			pIrp->IoStatus.Information = NULL;

			if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(WriteRequest))
			{
				LogMessage(L"Invalid parameter size for IOCTL_WriteMemory request.", ErrorNotification);
				ntDeviceIoControlRoutineStatus = STATUS_INVALID_PARAMETER;
				break;
			}

			WriteRequest* pWrite = reinterpret_cast<WriteRequest*>(pIrp->AssociatedIrp.SystemBuffer);
			if (!pWrite)
			{
				LogMessage(L"Null paramer for IOCTL_WriteMemory request.", ErrorNotification);
				ntDeviceIoControlRoutineStatus = STATUS_INVALID_PARAMETER;
				break;
			}

			PVOID pKernelBuffer = ExAllocatePool(PagedPool, pWrite->ullSize);
			if (!pKernelBuffer)
			{
				LogMessage(L"Failed to allocate kernel memory buffer for IOCTL_WriteMemory request.", ErrorNotification);
				ntDeviceIoControlRoutineStatus = STATUS_NO_MEMORY;
				break;
			}
			__try
			{
				PEPROCESS pHostProcess = nullptr;
				ntDeviceIoControlRoutineStatus = PsLookupProcessByProcessId(ULongToHandle(pWrite->dwBufferProcessId), &pHostProcess);
				if (NT_ERROR(ntDeviceIoControlRoutineStatus))
				{
					LogMessage(L"Provided buffer process id is not valid for IOCTL_WriteMemory request.", ErrorNotification);
					break;
				}

				__try
				{
					if (!ReadMemoryThroughPageTable(pWrite->pBuffer, reinterpret_cast<iEPROCESS*>(pHostProcess)->Pcb.DirectoryTableBase, pKernelBuffer, pWrite->ullSize))
					{
						LogMessage(L"An error occured when attempting to copy memory for IOCTL_WriteMemory request.", ErrorNotification);
						ntDeviceIoControlRoutineStatus = STATUS_INTERNAL_ERROR;
						break;
					}
				}
				__finally
				{
					if (pHostProcess)
					{
						ObDereferenceObject(pHostProcess);
					}
				}

				PEPROCESS pTargetProcess = nullptr;
				ntDeviceIoControlRoutineStatus = PsLookupProcessByProcessId(ULongToHandle(pWrite->dwAddressProcessId), &pTargetProcess);
				if (NT_ERROR(ntDeviceIoControlRoutineStatus))
				{
					LogMessage(L"Provided target process id is not valid for IOCTL_WriteMemory request.", ErrorNotification);
					break;
				}
				__try
				{
					if (!WriteMemoryThroughPageTable(pKernelBuffer, pWrite->pAddress, reinterpret_cast<iEPROCESS*>(pTargetProcess)->Pcb.DirectoryTableBase, pWrite->ullSize))
					{
						LogMessage(L"An error occured when att empting to write memory for IOCTL_WriteMemory request.", ErrorNotification);
						ntDeviceIoControlRoutineStatus = STATUS_INTERNAL_ERROR;
						break;
					}
				}
				__finally
				{
					if (pTargetProcess)
					{
						ObDereferenceObject(pTargetProcess);
					}
				}
			}
			__finally
			{
				if (pKernelBuffer)
				{
					ExFreePool(pKernelBuffer);
				}
			}

			break;
		}



		default:
		{
			LogMessage(L"Unknown control code received.", ErrorNotification);
			ntDeviceIoControlRoutineStatus	= STATUS_INVALID_DEVICE_REQUEST;
			pIrp->IoStatus.Information		= NULL;
			break;
		}

		}
	} while (false);

	

	if (NT_ERROR(ntDeviceIoControlRoutineStatus))
	{
		LogMessage(L"DeviceIoControlRoutine has finished with a error(s)", ErrorNotification);
	}
	else
	{
		LogMessage(L"DeviceIoControlRoutine has finished with no errors.", GenericNotificaton);
	}

	pIrp->IoStatus.Status = ntDeviceIoControlRoutineStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return ntDeviceIoControlRoutineStatus;
}