#include "stdafx.h"

#include "CommunicationStructures.h"
#include "KernelStructures.h"
#include "Log.h"
#include "Typedefs.h"



class TableEntry
{
private:
	ULONG64 ullEntry;

public:
	ULONG64 ullFormattedEntry;
	bool bValid;
	TableEntry(ULONG64 ullDefEntry, USHORT usOffset)
		: ullEntry(ullDefEntry)
	{
		bValid = ullDefEntry & 0b1;
		if (!bValid)
		{
			ullFormattedEntry = NULL;
			return;
		}

		ULONG64 ullBuffer = ullEntry;
		ullBuffer = ullBuffer >> 12;

		ULONG64 ullBase = ullBuffer & 0b111111111111111111111111111111111111;
		ullBase = ullBase << 12;

		ullFormattedEntry = ullBase + (usOffset * sizeof(ULONG64));
	}
};

class PageEntry
{
private:
	ULONG64 ullEntry;

public:
	ULONG64 ullFormattedEntry;
	bool bValid;
	PageEntry(ULONG64 ullDefEntry, USHORT usOffset)
		: ullEntry(ullDefEntry)
	{
		bValid = ullDefEntry & 0b1;
		if (!bValid)
		{
			ullFormattedEntry = NULL;
			return;
		}

		ULONG64 ullBuffer = ullEntry;
		ullBuffer = ullBuffer >> 12;

		ULONG64 ullBase = ullBuffer & 0b111111111111111111111111111111111111;
		ullBase = ullBase << 12;

		ullFormattedEntry = ullBase + usOffset;
	}
};

class VirtualAddressOffsets
{
private:
	PVOID pAddress;

public:
	USHORT usPML4Offset;
	USHORT usPDPTOffset;
	USHORT usPDTOffset;
	USHORT usPTOffset;
	USHORT usPageOffset;

	VirtualAddressOffsets(PVOID pVirtualAddress)
		: pAddress(pAddress)
	{
		ULONG64 ullAddress = reinterpret_cast<ULONG64>(pVirtualAddress);
		usPageOffset = ullAddress & 0b111111111111;

		ullAddress = ullAddress >> 12;
		usPTOffset = ullAddress & 0b111111111;

		ullAddress = ullAddress >> 9;
		usPDTOffset = ullAddress & 0b111111111;

		ullAddress = ullAddress >> 9;
		usPDPTOffset = ullAddress & 0b111111111;

		ullAddress = ullAddress >> 9;
		usPML4Offset = ullAddress & 0b111111111;
	}
};



PHYSICAL_ADDRESS ConvertToPhysicalAddress(ULONG64 ullPhysicalAddress)
{
	PHYSICAL_ADDRESS PA = {};
	PA.LowPart = ullPhysicalAddress & 0xFFFFFFFF;
	PA.HighPart = ullPhysicalAddress >> 32;

	return PA;
}

ULONG64 GetNextEntry(ULONG64 ullTablePhysicalAddress, USHORT usTableOffset)
{
	PHYSICAL_ADDRESS FormattedTablePhysicalAddress = ConvertToPhysicalAddress(ullTablePhysicalAddress);
	if (FormattedTablePhysicalAddress.HighPart == NULL && FormattedTablePhysicalAddress.LowPart == NULL)
	{
		return NULL;
	}

	PULONG64 pTable = reinterpret_cast<PULONG64>(MmMapIoSpace(FormattedTablePhysicalAddress, 1 << 12, MmNonCached));
	if (!pTable)
	{
		return NULL;
	}

	ULONG64 ullNextLevelEntry = NULL;
	__try
	{
		ullNextLevelEntry = *(pTable + usTableOffset);
	}
	__finally
	{
		MmUnmapIoSpace(pTable, 1 << 12);
	}

	return (ullNextLevelEntry);
}





ULONG64 FormatPTE(ULONG64 ullOldPTE, PTEFlags* pFlags)
{
	ULONG64 ullNewPTE = ullOldPTE;
	for (DWORD i = 0; i < ((sizeof(PTEFlags) / sizeof(bool)) - 1); i++)
	{
		switch (reinterpret_cast<BYTE*>(pFlags)[i])
		{

		case 0:		//Off
		{
			ullNewPTE &= ~(static_cast<ULONG64>(1) << i);
			break;
		}

		case 1:		//On
		{
			ullNewPTE |= static_cast<ULONG64>(1) << i;
			break;
		}

		default:	//Don't change
		{
			break;
		}

		}
	}

	switch (pFlags->bNoExecute)
	{

	case 0:
	{
		ullNewPTE &= ~(static_cast<ULONG64>(0b1) << 63);
		break;
	}

	case 1:
	{
		ullNewPTE |= static_cast<ULONG64>(0b1) << 63;
		break;
	}

	default:
	{
		break;
	}

	}

	return ullNewPTE;
}


PageTableEntry GetPageEntry(PVOID pVirtualAddress, ULONG64 paPML4, bool bModify, PTEFlags* pFlags = nullptr)
{
	VirtualAddressOffsets Offsets(pVirtualAddress);

	do
	{
		ULONG64 ullPML4Entry = GetNextEntry(paPML4, Offsets.usPML4Offset);
		if (!ullPML4Entry)
		{
			break;
		}
		TableEntry ePML4(ullPML4Entry, NULL);
		if (!ePML4.bValid)
		{
			LogMessage(L"PML4 Entry is not valid.", ErrorNotification);
			break;
		}

		ULONG64 ullPDPTEntry = GetNextEntry(ePML4.ullFormattedEntry, Offsets.usPDPTOffset);
		if (!ullPDPTEntry)
		{
			LogMessage(L"PDPT Entry is not valid.", ErrorNotification);
			break;
		}
		TableEntry ePDPT(ullPDPTEntry, NULL);

		ULONG64 ullPDEntry = GetNextEntry(ePDPT.ullFormattedEntry, Offsets.usPDTOffset);
		if (!ullPDEntry)
		{
			LogMessage(L"PD Entry is not valid.", ErrorNotification);
			break;
		}
		TableEntry ePD(ullPDEntry, NULL);

		if (bModify)
		{
			PHYSICAL_ADDRESS BasePTPhysicalAddress = ConvertToPhysicalAddress(ePD.ullFormattedEntry);
			if (BasePTPhysicalAddress.HighPart == NULL && BasePTPhysicalAddress.LowPart == NULL)
			{
				break;
			}

			PULONG64 pPT = reinterpret_cast<PULONG64>(MmMapIoSpace(BasePTPhysicalAddress, 1 << 12, MmNonCached));
			if (!pPT)
			{
				break;
			}

			__try
			{
				ULONG64 ullOldPTE = *(pPT + Offsets.usPTOffset);
				ULONG64 ullNewPTE = FormatPTE(ullOldPTE, pFlags);
				*(pPT + Offsets.usPTOffset) = ullNewPTE;
			}
			__finally
			{
				if (pPT)
				{
					MmUnmapIoSpace(pPT, 1 << 12);
				}
			}

			PageTableEntry ReturnMe;
			ReturnMe.ullPhysicalAddress = 1;
			ReturnMe.ullPTE				= 1;
			ReturnMe.ullVirtualAdddress = 1;
			return ReturnMe;
		}
		else
		{
			ULONG64 ullPTEntry = GetNextEntry(ePD.ullFormattedEntry, Offsets.usPTOffset);
			if (!ullPTEntry)
			{
				break;
			}

			PageEntry Page(ullPTEntry, Offsets.usPageOffset);	//Page + offset

			PageTableEntry ReturnMe;
			ReturnMe.ullPhysicalAddress = Page.ullFormattedEntry;
			ReturnMe.ullPTE				= ullPTEntry;
			ReturnMe.ullVirtualAdddress = reinterpret_cast<ULONG64>(pVirtualAddress);
			return ReturnMe;
		}

		
	} while (false);
	
	return {};
}


bool IsPageBitSet(PVOID pAddress, ULONG64 paPML4, PTEBits Bit)
{
	PageTableEntry Entry = GetPageEntry(pAddress, paPML4, false);

	return ((Entry.ullPTE >> Bit) & 0b1);
}


ULONG64 GetPageGranularityAddress(PVOID pAddress)
{
	VirtualAddressOffsets Offsets(pAddress);

	return (reinterpret_cast<ULONG64>(pAddress) - Offsets.usPageOffset);
}
