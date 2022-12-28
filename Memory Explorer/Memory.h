#pragma once

#include "stdafx.h"



bool ReadMemoryThroughPageTable(PVOID pAddress, ULONG64 paPML4, PVOID pBuffer, size_t ullSize);
bool WriteMemoryThroughPageTable(PVOID pBuffer, PVOID pTarget, ULONG64 paPML4, size_t ullSize);