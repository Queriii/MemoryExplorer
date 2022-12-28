#pragma once

#include "stdafx.h"

#include "Typedefs.h"




typedef struct PageLookupRequest //IOCTL_LookupPage, Arg
{
	PVOID pAddress;
	DWORD dwProcessId;
}PageLookupRequest;
typedef struct PageTableEntry	//IOCTL_LookupPage, Return
{
	ULONG64 ullPTE;

	ULONG64 ullVirtualAdddress;
	ULONG64 ullPhysicalAddress;
}PageTableEntry;

typedef struct PTEFlags
{
	BYTE bValid;
	BYTE bWrite;
	BYTE bOwner;
	BYTE bWriteThrough;
	BYTE bCacheDisabled;
	BYTE bAccessed;
	BYTE bDirty;
	BYTE bLargePage;
	BYTE bGlobal;
	BYTE bSftCopyOnWrite;
	BYTE bSftPrototype;
	BYTE bSftWrite;
	BYTE bNoExecute;
}PTEFlags;
typedef struct PageModificationRequest //IOCTL_ModifyPage, Arg
{
	PVOID	pAddress;
	DWORD	dwProcessId;

	PTEFlags Flags;
}PageModificationRequest;


typedef struct ReadRequest
{
	PVOID pAddress;
	size_t ullSize;
	DWORD dwProcessId;
}ReadRequest;

typedef struct WriteRequest
{
	PVOID pAddress;
	DWORD dwAddressProcessId;

	PVOID pBuffer;
	DWORD dwBufferProcessId;

	size_t ullSize;
}WriteRequest;