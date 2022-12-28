#pragma once

#include "stdafx.h"



typedef unsigned char BYTE;
typedef unsigned int DWORD;

//PTE bits
enum PTEBits
{
	Valid			= 0,
	Write			= 1,
	Owner			= 2,
	WriteThrough	= 3,
	CacheDisabled	= 4,
	Accessed		= 5,
	Dirty			= 6,
	LargePage		= 7,
	Global			= 8,
	SftCopyOnWrite	= 9,
	SftPrototype	= 10,
	SftWrite		= 11,
	NoExecute		= 63
};

constexpr DWORD dwSupportedPageSize = 1 << 12;	//This driver is only intended for normal page sizes, I'll probably add support for large and huge pages later.