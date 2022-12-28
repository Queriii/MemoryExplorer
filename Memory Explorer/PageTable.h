#pragma once

#include "stdafx.h"

#include "CommunicationStructures.h"



PageTableEntry GetPageEntry(PVOID pVirtualAddress, ULONG64 paPML4, bool bModify, PTEFlags* pFlags = nullptr);

bool IsPageBitSet(PVOID pAddress, ULONG64 paPML4, PTEBits Bit);

ULONG64 GetPageGranularityAddress(PVOID pAddress);

PHYSICAL_ADDRESS ConvertToPhysicalAddress(ULONG64 ullPhysicalAddress);