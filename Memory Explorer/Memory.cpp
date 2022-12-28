#include "stdafx.h"

#include "PageTable.h"
#include "Log.h"



DWORD GetNumberOfPagesInRange(PVOID pStart, size_t ullSize)
{
	DWORD	dwPagesToRead = 0;
	for (ULONG64 ullCurrentAddress = reinterpret_cast<ULONG64>(pStart), ullEndingAddress = reinterpret_cast<ULONG64>(pStart) + ullSize; ullCurrentAddress < ullEndingAddress; ullCurrentAddress += dwSupportedPageSize - (ullCurrentAddress - GetPageGranularityAddress(reinterpret_cast<PVOID>(ullCurrentAddress))))
	{
		dwPagesToRead++;
	}
	return dwPagesToRead;
}


bool ReadMemoryThroughPageTable(PVOID pAddress, ULONG64 paPML4, PVOID pBuffer, size_t ullSize)
{
	DWORD	dwPagesToRead = GetNumberOfPagesInRange(pAddress, ullSize);

	ULONG64 ullAddressToVerify	= reinterpret_cast<ULONG64>(pAddress);
	ULONG64 ullByteDifference	= (reinterpret_cast<ULONG64>(pAddress) - GetPageGranularityAddress(pAddress));
	ULONG64	ullBytesToRead		= 0;
	if (dwPagesToRead == 1)
	{
		ullBytesToRead = ullSize;
	}
	else
	{
		ullBytesToRead = dwSupportedPageSize - (reinterpret_cast<ULONG64>(pAddress) - GetPageGranularityAddress(pAddress));
	}
	
	LONG64	llCheck			= ullSize;
	ULONG64 ullBufferOffset = 0;
	bool bInvalidPage = false;
	for (DWORD i = 0; i < dwPagesToRead; i++)
	{
		if (!IsPageBitSet(reinterpret_cast<PVOID>(ullAddressToVerify), paPML4, PTEBits::Valid))
		{
			LogMessage(L"Invalid memory region provided for request.", ErrorNotification);
			bInvalidPage = true;
			break;
		}

		PageTableEntry Entry = GetPageEntry(reinterpret_cast<PVOID>(ullAddressToVerify), paPML4, false);
		if (!Entry.ullPhysicalAddress || !Entry.ullPTE || !Entry.ullVirtualAdddress)
		{
			LogMessage((L"Invalid PageTableEntry state for request."), ErrorNotification);
			bInvalidPage = true;
			break;
		}
		PHYSICAL_ADDRESS EntryPhysicalAddress = ConvertToPhysicalAddress(Entry.ullPhysicalAddress);
		if (!EntryPhysicalAddress.LowPart && !EntryPhysicalAddress.HighPart)
		{
			LogMessage(L"Failed to convert PageTableEntry into proper PHYSICAL_ADDRESS form for request.", ErrorNotification);
			bInvalidPage = true;
			break;
		}
		PVOID pVirtualPage = MmMapIoSpace(EntryPhysicalAddress, ullBytesToRead, MmNonCached);
		if (!pVirtualPage)
		{
			LogMessage(L"Failed to map page into virtual memory for request.", ErrorNotification);
			bInvalidPage = true;
			break;
		}

		__try
		{
			memcpy_s(reinterpret_cast<BYTE*>(pBuffer) + ullBufferOffset, ullSize, pVirtualPage, ullBytesToRead);
		}
		__finally
		{
			MmUnmapIoSpace(pVirtualPage, ullBytesToRead);
		}

		ullBufferOffset += ullBytesToRead;
		llCheck -= dwSupportedPageSize - ullByteDifference;
		if (llCheck > dwSupportedPageSize)
		{
			ullBytesToRead = dwSupportedPageSize;
		}
		else
		{
			ullBytesToRead = llCheck;
		}
		ullAddressToVerify += (dwSupportedPageSize - ullByteDifference);
		ullByteDifference = 0;
	}

	return !bInvalidPage;
}



bool WriteMemoryThroughPageTable(PVOID pBuffer, PVOID pTarget, ULONG64 paPML4, size_t ullSize)
{
	DWORD dwPagesToWrite = GetNumberOfPagesInRange(pTarget, ullSize);

	ULONG64 ullAddressToVerify	= reinterpret_cast<ULONG64>(pTarget);
	ULONG64 ullByteDifference	= (reinterpret_cast<ULONG64>(pTarget) - GetPageGranularityAddress(pTarget));
	ULONG64	ullBytesToWrite		= 0;
	if (dwPagesToWrite == 1)
	{
		ullBytesToWrite = ullSize;
	}
	else
	{
		ullBytesToWrite = dwSupportedPageSize - (reinterpret_cast<ULONG64>(pTarget) - GetPageGranularityAddress(pTarget));
	}

	LONG64	llCheck			= ullSize;
	ULONG64 ullBufferOffset = 0;
	bool	bInvalidPage	= false;
	for (DWORD i = 0; i < dwPagesToWrite; i++)
	{
		if (!IsPageBitSet(reinterpret_cast<PVOID>(ullAddressToVerify), paPML4, PTEBits::Valid))
		{
			LogMessage(L"Invalid memory region provided for request.", ErrorNotification);
			bInvalidPage = true;
			break;
		}

		if (!IsPageBitSet(reinterpret_cast<PVOID>(ullAddressToVerify), paPML4, PTEBits::SftWrite))
		{
			LogMessage(L"Memory region not writeable for request.", ErrorNotification);
			bInvalidPage = true;
			break;
		}

		PageTableEntry Entry = GetPageEntry(reinterpret_cast<PVOID>(ullAddressToVerify), paPML4, false);
		if (!Entry.ullPhysicalAddress || !Entry.ullPTE || !Entry.ullVirtualAdddress)
		{
			LogMessage((L"Invalid PageTableEntry state for request."), ErrorNotification);
			bInvalidPage = true;
			break;
		}
		PHYSICAL_ADDRESS EntryPhysicalAddress = ConvertToPhysicalAddress(Entry.ullPhysicalAddress);
		if (!EntryPhysicalAddress.LowPart && !EntryPhysicalAddress.HighPart)
		{
			LogMessage(L"Failed to convert PageTableEntry into proper PHYSICAL_ADDRESS form for request.", ErrorNotification);
			bInvalidPage = true;
			break;
		}
		PVOID pVirtualPage = MmMapIoSpace(EntryPhysicalAddress, ullBytesToWrite, MmNonCached);
		if (!pVirtualPage)
		{
			LogMessage(L"Failed to map page into virtual memory for request.", ErrorNotification);
			bInvalidPage = true;
			break;
		}

		__try
		{
			memcpy_s(pVirtualPage, ullBytesToWrite, reinterpret_cast<BYTE*>(pBuffer) + ullBufferOffset, ullBytesToWrite);
		}
		__finally
		{
			MmUnmapIoSpace(pVirtualPage, ullBytesToWrite);
		}

		ullBufferOffset += ullBytesToWrite;
		llCheck -= dwSupportedPageSize - ullByteDifference;
		if (llCheck > dwSupportedPageSize)
		{
			ullBytesToWrite = dwSupportedPageSize;
		}
		else
		{
			ullBytesToWrite = llCheck;
		}
		ullAddressToVerify += (dwSupportedPageSize - ullByteDifference);
		ullByteDifference = 0;
	}

	return !bInvalidPage;
}