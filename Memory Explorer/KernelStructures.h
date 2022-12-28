#pragma once

#include "stdafx.h"

#include "Typedefs.h"



typedef struct iKPROCESS
{
	BYTE	Padding[0x28];
	ULONG64	DirectoryTableBase;
}iKPROCESS;

typedef struct iEPROCESS
{
	iKPROCESS Pcb;
}iEPROCESS;